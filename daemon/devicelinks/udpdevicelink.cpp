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

#include "udpdevicelink.h"
#include "announcers/avahiannouncer.h"

UdpDeviceLink::UdpDeviceLink(const QString& d, AvahiAnnouncer* a, QHostAddress ip, quint16 port)
    : DeviceLink(d, a)
{

    mIp = ip;
    mPort = port;

    mUdpSocket = new QUdpSocket();
    mUdpSocket->bind(port);
    connect(mUdpSocket, SIGNAL(readyRead()), this, SLOT(readPendingNotifications()));

}

void UdpDeviceLink::readPendingNotifications()
{

    qDebug() << "UdpDeviceLink readPendingNotifications";

    while (mUdpSocket->hasPendingDatagrams()) {

        QByteArray datagram;
        datagram.resize(mUdpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        mUdpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        //log.write(datagram);
        qDebug() << datagram;

        NetworkPackage np = NetworkPackage::fromString(datagram);

        emit receivedPackage(np);
        
    }

}
