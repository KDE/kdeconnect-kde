/**
 * Copyright 2018 Erik Duisters <e.duisters1@gmail.com>
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

#ifndef COMPOSITEUPLOADJOB_H
#define COMPOSITEUPLOADJOB_H

#include "kdeconnectcore_export.h"
#include <KCompositeJob>
#include "server.h"
#include "uploadjob.h"

class KDECONNECTCORE_EXPORT CompositeUploadJob
    : public KCompositeJob
{
    Q_OBJECT

public:
    explicit CompositeUploadJob(const QString& deviceId, bool displayNotification);
    
    void start() override;
    QVariantMap transferInfo();
    bool isRunning();
    bool addSubjob(KJob* job) override;
    
private:
    bool startListening();
    void emitDescription(const QString& currentFileName);
    
protected:
    bool doKill() override;

private:
    enum {
        NoPortAvailable = UserDefinedError,
        SendingNetworkPacketFailed,
        SocketError,
        SslError
    };
    
    Server *const m_server;
    QSslSocket *m_socket;
    quint16 m_port;
    const QString& m_deviceId;
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
    void socketDisconnected();
    void socketError(QAbstractSocket::SocketError socketError);
    void sslError(const QList<QSslError>& errors);
    void encrypted();
    void slotProcessedAmount(KJob *job, KJob::Unit unit, qulonglong amount);
    void slotResult(KJob *job) override;
    void startNextSubJob();
    void sendUpdatePacket();
};

#endif //COMPOSITEUPLOADJOB_H
