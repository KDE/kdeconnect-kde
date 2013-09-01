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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "landevicelink.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

#include "linkproviders/linkprovider.h"
#include "networkpackage.h"

LanDeviceLink::LanDeviceLink(const QString& d, LinkProvider* a, QTcpSocket* socket)
    : DeviceLink(d, a)
{
    mSocket = socket;

    int fd = socket->socketDescriptor();
    int enableKeepAlive = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enableKeepAlive, sizeof(enableKeepAlive));

    int maxIdle = 60; /* seconds */
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &maxIdle, sizeof(maxIdle));

    int count = 3;  // send up to 3 keepalive packets out, then disconnect if no response
    setsockopt(fd, SOL_TCP, TCP_KEEPCNT, &count, sizeof(count));

    int interval = 5;   // send a keepalive packet out every 2 seconds (after the 5 second idle period)
    setsockopt(fd, SOL_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));

    connect(mSocket, SIGNAL(disconnected()),
            this, SLOT(deleteLater()));
    connect(mSocket, SIGNAL(readyRead()),
            this, SLOT(dataReceived()));
}

bool LanDeviceLink::sendPackage(const NetworkPackage& np)
{
    int written = mSocket->write(np.serialize());
    return written != -1;
}

void LanDeviceLink::dataReceived()
{
    qDebug() << "LanDeviceLink dataReceived";

    QByteArray data = mSocket->readAll();
    QList<QByteArray> packages = data.split('\n');
    Q_FOREACH(const QByteArray& package, packages) {

        if (package.length() < 3) continue;

        NetworkPackage np("");
        NetworkPackage::unserialize(package, &np);

        Q_EMIT receivedPackage(np);

    }
}
