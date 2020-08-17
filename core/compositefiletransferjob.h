/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef COMPOSITEFILETRANSFERJOB_H
#define COMPOSITEFILETRANSFERJOB_H

#include "kdeconnectcore_export.h"
#include <KCompositeJob>
#include <QElapsedTimer>

class FileTransferJob;

class KDECONNECTCORE_EXPORT CompositeFileTransferJob
    : public KCompositeJob
{
    Q_OBJECT

public:
    explicit CompositeFileTransferJob(const QString& deviceId);

    void start() override;
    bool isRunning() const;
    bool addSubjob(KJob* job) override;

protected:
    bool doKill() override;

private Q_SLOTS:
    void slotProcessedAmount(KJob *job, KJob::Unit unit, qulonglong amount);
    void slotResult(KJob *job) override;
    void startNextSubJob();

private:
    void emitDescription(const QString& currentFileName);

    QString m_deviceId;
    bool m_running;
    int m_currentJobNum;
    int m_totalJobs;
    quint64 m_currentJobSendPayloadSize;
    quint64 m_totalSendPayloadSize;
    quint64 m_totalPayloadSize;
    FileTransferJob *m_currentJob;
    QElapsedTimer m_timer;
    quint64 m_prevElapsedTime;

};

#endif //COMPOSITEFILETRANSFERJOB_H

