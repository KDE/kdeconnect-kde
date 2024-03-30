/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "compositefiletransferjob.h"
#include "filetransferjob.h"
#include <KLocalizedString>
#include <core_debug.h>
#include <daemon.h>

CompositeFileTransferJob::CompositeFileTransferJob(const QString &deviceId)
    : KCompositeJob()
    , m_deviceId(deviceId)
    , m_running(false)
    , m_currentJobNum(1)
    , m_totalJobs(0)
    , m_currentJobSentPayloadSize(0)
    , m_totalSentPayloadSize(0)
    , m_oldTotalSentPayloadSize(0)
    , m_totalPayloadSize(0)
    , m_currentJob(nullptr)
{
    setCapabilities(Killable);
}

bool CompositeFileTransferJob::isRunning() const
{
    return m_running;
}

void CompositeFileTransferJob::start()
{
    QMetaObject::invokeMethod(this, "startNextSubJob", Qt::QueuedConnection);
    m_running = true;
}

void CompositeFileTransferJob::startNextSubJob()
{
    m_currentJob = qobject_cast<FileTransferJob *>(subjobs().at(0));
    m_currentJobSentPayloadSize = 0;

    m_currentJob->start();
    Q_EMIT description(this,
                       i18ncp("@title job", "Receiving file", "Receiving files", m_totalJobs),
                       {i18nc("The source of a file operation", "Source"), Daemon::instance()->getDevice(this->m_deviceId)->name()},
                       {i18nc("The destination of a file operation", "Destination"), m_currentJob->destination().toDisplayString(QUrl::PreferLocalFile)});

    connect(m_currentJob, &FileTransferJob::processedAmountChanged, this, &CompositeFileTransferJob::slotProcessedAmount);
}

bool CompositeFileTransferJob::addSubjob(KJob *job)
{
    if (FileTransferJob *uploadJob = qobject_cast<FileTransferJob *>(job)) {
        const NetworkPacket *np = uploadJob->networkPacket();

        if (np->has(QStringLiteral("totalPayloadSize"))) {
            m_totalPayloadSize = np->get<quint64>(QStringLiteral("totalPayloadSize"));
            setTotalAmount(Bytes, m_totalPayloadSize);
        }

        if (np->has(QStringLiteral("numberOfFiles"))) {
            m_totalJobs = np->get<int>(QStringLiteral("numberOfFiles"));
            setTotalAmount(Files, m_totalJobs);
        }

        if (!hasSubjobs()) {
            QMetaObject::invokeMethod(this, "startNextSubJob", Qt::QueuedConnection);
        }

        return KCompositeJob::addSubjob(job);
    } else {
        qCDebug(KDECONNECT_CORE) << "CompositeFileTransferJob::addSubjob() - you can only add FileTransferJob's, ignoring";
        return false;
    }
    return true;
}

bool CompositeFileTransferJob::doKill()
{
    m_running = false;
    if (m_currentJob) {
        return m_currentJob->kill();
    }
    return true;
}

void CompositeFileTransferJob::slotProcessedAmount(KJob * /*job*/, KJob::Unit unit, qulonglong amount)
{
    m_currentJobSentPayloadSize = amount;
    const auto totalSent = m_totalSentPayloadSize + m_currentJobSentPayloadSize;

    if (!m_reportTimer.isValid()) {
        m_reportTimer.start();
    }
    if (m_reportTimer.hasExpired(250)) {
        setProcessedAmount(unit, totalSent);
        m_reportTimer.restart();
    }

    if (!m_speedTimer.isValid()) {
        m_speedTimer.start();
    }
    if (m_speedTimer.hasExpired(1000)) {
        emitSpeed(1000 * (totalSent - m_oldTotalSentPayloadSize) / m_speedTimer.elapsed());
        m_oldTotalSentPayloadSize = totalSent;
        m_speedTimer.restart();
    }
}

void CompositeFileTransferJob::slotResult(KJob *job)
{
    // Copies job error and errorText and emits result if job is in error otherwise removes job from subjob list
    KCompositeJob::slotResult(job);

    if (error() || !m_running) {
        return;
    }

    m_totalSentPayloadSize += m_currentJobSentPayloadSize;

    setProcessedAmount(Bytes, m_totalSentPayloadSize);
    setProcessedAmount(Files, m_currentJobNum);

    if (m_currentJobNum < m_totalJobs) {
        m_currentJobNum++;
        if (!subjobs().empty()) {
            startNextSubJob();
        }
    } else {
        emitResult();
    }
}

#include "moc_compositefiletransferjob.cpp"
