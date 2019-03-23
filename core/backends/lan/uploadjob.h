/*
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef UPLOADJOB_H
#define UPLOADJOB_H

#include <KJob>

#include <QIODevice>
#include <QVariantMap>
#include <QSslSocket>
#include "server.h"
#include <QElapsedTimer>
#include <networkpacket.h>

class KDECONNECTCORE_EXPORT UploadJob
    : public KJob
{
    Q_OBJECT
public:
    explicit UploadJob(const NetworkPacket& networkPacket);

    void setSocket(QSslSocket* socket);
    void start() override;
    bool stop();
    const NetworkPacket getNetworkPacket();

private:
    const NetworkPacket m_networkPacket;
    QSharedPointer<QIODevice> m_input;
    QSslSocket* m_socket;
    qint64 bytesUploading;
    qint64 bytesUploaded;

    const static quint16 MIN_PORT = 1739;
    const static quint16 MAX_PORT = 1764;
    
private Q_SLOTS:
    void uploadNextPacket();
    void encryptedBytesWritten(qint64 bytes);
    void aboutToClose();
};

#endif // UPLOADJOB_H
