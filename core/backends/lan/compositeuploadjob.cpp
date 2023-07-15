/**
 * SPDX-FileCopyrightText: 2018 Erik Duisters <e.duisters1@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "compositeuploadjob.h"
#include "lanlinkprovider.h"
#include "plugins/share/shareplugin.h"
#include "qtcompat_p.h"
#include <KJobTrackerInterface>
#include <KLocalizedString>
#include <core_debug.h>
#include <daemon.h>

#ifdef HAVE_KIO
#include <kio/global.h>
#endif

CompositeUploadJob::CompositeUploadJob(const QString &deviceId, bool displayNotification)
    : KCompositeJob()
    , m_server(new Server(this))
    , m_socket(nullptr)
    , m_port(0)
    , m_deviceId(deviceId)
    , m_running(false)
    , m_currentJobNum(1)
    , m_totalJobs(0)
    , m_currentJobSendPayloadSize(0)
    , m_totalSendPayloadSize(0)
    , m_totalPayloadSize(0)
    , m_currentJob(nullptr)
    , m_prevElapsedTime(0)
    , m_updatePacketPending(false)
{
    setCapabilities(Killable);

    if (displayNotification) {
        Daemon::instance()->jobTracker()->registerJob(this);
    }
}

bool CompositeUploadJob::isRunning()
{
    return m_running;
}

void CompositeUploadJob::start()
{
    if (m_running) {
        qCWarning(KDECONNECT_CORE) << "CompositeUploadJob::start() - already running";
        return;
    }

    if (!hasSubjobs()) {
        qCWarning(KDECONNECT_CORE) << "CompositeUploadJob::start() - there are no subjobs to start";
        emitResult();
        return;
    }

    if (!startListening()) {
        return;
    }

    connect(m_server, &QTcpServer::newConnection, this, &CompositeUploadJob::newConnection);

    m_running = true;

    // Give SharePlugin some time to add subjobs
    QMetaObject::invokeMethod(
        Daemon::instance()->getDevice(m_deviceId),
        [this] {
            startNextSubJob();
        },
        Qt::QueuedConnection);
}

bool CompositeUploadJob::startListening()
{
    m_port = MIN_PORT;
    while (!m_server->listen(QHostAddress::Any, m_port)) {
        m_port++;
        if (m_port > MAX_PORT) { // No ports available?
            qCWarning(KDECONNECT_CORE) << "CompositeUploadJob::startListening() - Error opening a port in range" << MIN_PORT << "-" << MAX_PORT;
            m_port = 0;
            setError(NoPortAvailable);
            setErrorText(i18n("Couldn't find an available port"));
            emitResult();
            return false;
        }
    }

    qCDebug(KDECONNECT_CORE) << "CompositeUploadJob::startListening() - listening on port: " << m_port;
    return true;
}

void CompositeUploadJob::startNextSubJob()
{
    m_currentJob = qobject_cast<UploadJob *>(subjobs().at(0));
    m_currentJobSendPayloadSize = 0;
    emitDescription(m_currentJob->getNetworkPacket().get<QString>(QStringLiteral("filename")));

#ifdef SAILFISHOS
    connect(m_currentJob, SIGNAL(processedAmount(KJob *, KJob::Unit, qulonglong)), this, SLOT(slotProcessedAmount(KJob *, KJob::Unit, qulonglong)));
#else
    connect(m_currentJob, QOverload<KJob *, KJob::Unit, qulonglong>::of(&UploadJob::processedAmount), this, &CompositeUploadJob::slotProcessedAmount);
#endif
    // Already done by KCompositeJob
    // connect(m_currentJob, &KJob::result, this, &CompositeUploadJob::slotResult);

    // TODO: Create a copy of the networkpacket that can be re-injected if sending via lan fails?
    NetworkPacket np = m_currentJob->getNetworkPacket();
#if QT_VERSION < QT_VERSION_CHECK(5, 8, 0)
    np.setPayload({}, np.payloadSize());
#else
    np.setPayload(nullptr, np.payloadSize());
#endif
    np.setPayloadTransferInfo({{QStringLiteral("port"), m_port}});
    np.set<int>(QStringLiteral("numberOfFiles"), m_totalJobs);
    np.set<quint64>(QStringLiteral("totalPayloadSize"), m_totalPayloadSize);

    if (Daemon::instance()->getDevice(m_deviceId)->sendPacket(np)) {
        m_server->resumeAccepting();
    } else {
        setError(SendingNetworkPacketFailed);
        setErrorText(i18n("Failed to send packet to %1", Daemon::instance()->getDevice(m_deviceId)->name()));

        emitResult();
    }
}

void CompositeUploadJob::newConnection()
{
    m_server->pauseAccepting();

    m_socket = m_server->nextPendingConnection();

    if (!m_socket) {
        qCDebug(KDECONNECT_CORE) << "CompositeUploadJob::newConnection() - m_server->nextPendingConnection() returned a nullptr";
        return;
    }

    m_currentJob->setSocket(m_socket);

    connect(m_socket, &QSslSocket::disconnected, this, &CompositeUploadJob::socketDisconnected);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::error), this, &CompositeUploadJob::socketError);
#else
    connect(m_socket, &QAbstractSocket::errorOccurred, this, &CompositeUploadJob::socketError);
#endif
    connect(m_socket, QOverload<const QList<QSslError> &>::of(&QSslSocket::sslErrors), this, &CompositeUploadJob::sslError);
    connect(m_socket, &QSslSocket::encrypted, this, &CompositeUploadJob::encrypted);

    LanLinkProvider::configureSslSocket(m_socket, m_deviceId, true);

    m_socket->startServerEncryption();
}

void CompositeUploadJob::socketDisconnected()
{
    m_socket->close();
}

void CompositeUploadJob::socketError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error);

    // Do not close the socket because when android closes the socket (share is cancelled) closing the socket leads to a cyclic socketError and eventually a
    // segv
    setError(SocketError);
    emitResult();

    m_running = false;
}

void CompositeUploadJob::sslError(const QList<QSslError> &errors)
{
    Q_UNUSED(errors);

    m_socket->close();
    setError(SslError);
    emitResult();

    m_running = false;
}

void CompositeUploadJob::encrypted()
{
    if (!m_timer.isValid()) {
        m_timer.start();
    }

    m_currentJob->start();
}

bool CompositeUploadJob::addSubjob(KJob *job)
{
    if (UploadJob *uploadJob = qobject_cast<UploadJob *>(job)) {
        NetworkPacket np = uploadJob->getNetworkPacket();

        m_totalJobs++;

        if (np.payloadSize() >= 0) {
            m_totalPayloadSize += np.payloadSize();
            setTotalAmount(Bytes, m_totalPayloadSize);
        }

        QString filename;
        QString filenameArg = QStringLiteral("filename");

        if (m_currentJob) {
            filename = m_currentJob->getNetworkPacket().get<QString>(filenameArg);
        } else {
            filename = np.get<QString>(filenameArg);
        }

        emitDescription(filename);

        if (m_running && m_currentJob && !m_updatePacketPending) {
            m_updatePacketPending = true;
            QMetaObject::invokeMethod(this, "sendUpdatePacket", Qt::QueuedConnection);
        }

        return KCompositeJob::addSubjob(job);
    } else {
        qCDebug(KDECONNECT_CORE) << "CompositeUploadJob::addSubjob() - you can only add UploadJob's, ignoring";
        return false;
    }
}

void CompositeUploadJob::sendUpdatePacket()
{
    NetworkPacket np(PACKET_TYPE_SHARE_REQUEST_UPDATE);
    np.set<int>(QStringLiteral("numberOfFiles"), m_totalJobs);
    np.set<quint64>(QStringLiteral("totalPayloadSize"), m_totalPayloadSize);

    Daemon::instance()->getDevice(m_deviceId)->sendPacket(np);

    m_updatePacketPending = false;
}

bool CompositeUploadJob::doKill()
{
    if (m_running) {
        m_running = false;

        return m_currentJob->stop();
    }

    return true;
}

void CompositeUploadJob::slotProcessedAmount(KJob *job, KJob::Unit unit, qulonglong amount)
{
    Q_UNUSED(job);

    m_currentJobSendPayloadSize = amount;
    quint64 uploaded = m_totalSendPayloadSize + m_currentJobSendPayloadSize;

    if (uploaded == m_totalPayloadSize || m_prevElapsedTime == 0 || m_timer.elapsed() - m_prevElapsedTime >= 100) {
        m_prevElapsedTime = m_timer.elapsed();
        setProcessedAmount(unit, uploaded);

        const auto elapsed = m_timer.elapsed();
        if (elapsed > 0) {
            emitSpeed((1000 * uploaded) / elapsed);
        }
    }
}

void CompositeUploadJob::slotResult(KJob *job)
{
    // Copies job error and errorText and emits result if job is in error otherwise removes job from subjob list
    KCompositeJob::slotResult(job);

    if (error() || !m_running) {
        return;
    }

    m_totalSendPayloadSize += m_currentJobSendPayloadSize;

    if (hasSubjobs()) {
        m_currentJobNum++;
        startNextSubJob();
    } else {
        emitResult();
    }
}

void CompositeUploadJob::emitDescription(const QString &currentFileName)
{
    Q_EMIT description(this, i18n("Sending to %1", Daemon::instance()->getDevice(this->m_deviceId)->name()), {i18n("File"), currentFileName}, {});

    setProcessedAmount(Files, m_currentJobNum);
    setTotalAmount(Files, m_totalJobs);
}
