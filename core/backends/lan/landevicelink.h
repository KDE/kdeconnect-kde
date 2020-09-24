/**
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

#ifndef LANDEVICELINK_H
#define LANDEVICELINK_H

#include <QObject>
#include <QPointer>
#include <QString>
#include <QSslSocket>
#include <QSslCertificate>

#include <kdeconnectcore_export.h>
#include "backends/devicelink.h"
#include "uploadjob.h"
#include "compositeuploadjob.h"

class SocketLineReader;

class KDECONNECTCORE_EXPORT LanDeviceLink
    : public DeviceLink
{
    Q_OBJECT

public:
    enum ConnectionStarted : bool { Locally, Remotely };

    LanDeviceLink(const QString& deviceId, LinkProvider* parent, QSslSocket* socket, ConnectionStarted connectionSource);
    void reset(QSslSocket* socket, ConnectionStarted connectionSource);

    QString name() override;
    bool sendPacket(NetworkPacket& np) override;

    void userRequestsPair() override;
    void userRequestsUnpair() override;

    void setPairStatus(PairStatus status) override;

    bool linkShouldBeKeptAlive() override;

    QHostAddress hostAddress() const;
    QSslCertificate certificate() const;

private Q_SLOTS:
    void dataReceived();

private:
    SocketLineReader* m_socketLineReader;
    ConnectionStarted m_connectionSource;
    QHostAddress m_hostAddress;
    QPointer<CompositeUploadJob> m_compositeUploadJob;
};

#endif
