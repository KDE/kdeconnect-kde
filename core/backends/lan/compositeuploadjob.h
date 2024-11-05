/**
 * SPDX-FileCopyrightText: 2018 Erik Duisters <e.duisters1@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef COMPOSITEUPLOADJOB_H
#define COMPOSITEUPLOADJOB_H

#include "kdeconnectcore_export.h"
#include "server.h"
#include "uploadjob.h"
#include <KCompositeJob>

class Device;

class KDECONNECTCORE_EXPORT CompositeUploadJob : public KCompositeJob
{
    Q_OBJECT

public:
    explicit CompositeUploadJob(Device *device, bool displayNotification);

    void start() override;
    QVariantMap transferInfo();
    bool isRunning();
    bool addSubjob(KJob *job) override;

private:
    bool startListening();
    void emitDescription(const QString &currentFileName);

protected:
    bool doKill() override;

private:
    enum {
        NoPortAvailable = UserDefinedError,
        SendingNetworkPacketFailed,
        SocketError,
        SslError,
    };

    Server *const m_server;
    QSslSocket *m_socket;
    quint16 m_port;
    Device *m_device;
    bool m_running;
    int m_currentJobNum;
    int m_totalJobs;
    quint64 m_currentJobSendPayloadSize;
    quint64 m_totalSendPayloadSize;
    quint64 m_totalPayloadSize;
    UploadJob *m_currentJob;
    QElapsedTimer m_timer;
    quint64 m_prevElapsedTime;
    bool m_updatePacketPending;

    const static quint16 MIN_PORT = 1739;
    const static quint16 MAX_PORT = 1764;

private Q_SLOTS:
    void newConnection();
    void slotProcessedAmount(KJob *job, KJob::Unit unit, qulonglong amount);
    void slotResult(KJob *job) override;
    void startNextSubJob();
    void sendUpdatePacket();
};

#endif // COMPOSITEUPLOADJOB_H
