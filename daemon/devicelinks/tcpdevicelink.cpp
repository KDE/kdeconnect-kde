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

#include "tcpdevicelink.h"
#include "linkproviders/avahitcplinkprovider.h"

TcpDeviceLink::TcpDeviceLink(const QString& d, AvahiTcpLinkProvider* a, QTcpSocket* socket)
    : DeviceLink(d, a)
{
    mSocket = socket;
    connect(mSocket, SIGNAL(disconnected()), this, SLOT(deleteLater()));
    connect(mSocket, SIGNAL(readyRead()), this, SLOT(dataReceived()));
}

bool TcpDeviceLink::sendPackage(const NetworkPackage& np)
{
    int written = mSocket->write(np.serialize());
    return written != -1;
}

void TcpDeviceLink::dataReceived()
{
    qDebug() << "TcpDeviceLink dataReceived";

    QByteArray data = mSocket->readAll();
    QList<QByteArray> packages = data.split('\n');
    Q_FOREACH(const QByteArray& package, packages) {

        if (package.length() < 3) continue;

        NetworkPackage np;
        NetworkPackage::unserialize(package,&np);

        emit receivedPackage(np);

    }
}
