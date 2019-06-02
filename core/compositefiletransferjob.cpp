/**
 * Copyright 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "compositefiletransferjob.h"
#include <core_debug.h>
#include <KLocalizedString>
#include <kio/global.h>
#include <KJobTrackerInterface>
#include <daemon.h>
#include "filetransferjob.h"

CompositeFileTransferJob::CompositeFileTransferJob(const QString& deviceId)
    : KCompositeJob()
    , m_deviceId(deviceId)
    , m_running(false)
    , m_currentJobNum(1)
    , m_totalJobs(0)
    , m_currentJobSendPayloadSize(0)
    , m_totalSendPayloadSize(0)
    , m_totalPayloadSize(0)
    , m_currentJob(nullptr)
    , m_prevElapsedTime(0)
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
    m_currentJob = qobject_cast<FileTransferJob*>(subjobs().at(0));
    m_currentJobSendPayloadSize = 0;
    emitDescription(m_currentJob->destination().toString());
    m_currentJob->start();
    connect(m_currentJob, QOverload<KJob*,KJob::Unit,qulonglong>::of(&FileTransferJob::processedAmount), this, &CompositeFileTransferJob::slotProcessedAmount);
}

bool CompositeFileTransferJob::addSubjob(KJob* job)
{
    if (FileTransferJob *uploadJob = qobject_cast<FileTransferJob*>(job)) {

        const NetworkPacket* np = uploadJob->networkPacket();

        if (np->has(QStringLiteral("totalPayloadSize"))) {
            m_totalPayloadSize = np->get<quint64>(QStringLiteral("totalPayloadSize"));
            setTotalAmount(Bytes, m_totalPayloadSize);
        }

        if (np->has(QStringLiteral("numberOfFiles"))) {
            m_totalJobs = np->get<int>(QStringLiteral("numberOfFiles"));
        }

        QString filename = np->get<QString>(QStringLiteral("filename"));
        emitDescription(filename);

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

void CompositeFileTransferJob::slotProcessedAmount(KJob *job, KJob::Unit unit, qulonglong amount)
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

void CompositeFileTransferJob::slotResult(KJob *job)
{
    //Copies job error and errorText and emits result if job is in error otherwise removes job from subjob list
    KCompositeJob::slotResult(job);

    if (error() || !m_running) {
        return;
    }

    m_totalSendPayloadSize += m_currentJobSendPayloadSize;

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

void CompositeFileTransferJob::emitDescription(const QString& currentFileName)
{
    QPair<QString, QString> field2;

    const QUrl fileUrl(currentFileName);
    const QString fileName = fileUrl.toDisplayString(QUrl::PreferLocalFile);

    if (m_totalJobs > 1) {
        field2.first = i18n("Progress");
        field2.second = i18n("Receiving file %1 of %2", m_currentJobNum, m_totalJobs);
    }

    Q_EMIT description(this, i18np("Receiving file from %2", "Receiving %1 files from %2", m_totalJobs, Daemon::instance()->getDevice(this->m_deviceId)->name()),
                       { i18n("File"), fileName }, field2
    );
}

