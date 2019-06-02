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

