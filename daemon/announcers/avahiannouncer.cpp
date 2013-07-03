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

#include "avahiannouncer.h"

#include "devicelinks/udpdevicelink.h"

#include <QHostInfo>

AvahiAnnouncer::AvahiAnnouncer()
{
    QString serviceType = "_kdeconnect._udp";
    quint16 port = 10601;

    //http://api.kde.org/4.x-api/kdelibs-apidocs/dnssd/html/index.html

    service = new DNSSD::PublicService(QHostInfo::localHostName(), serviceType, port);

    mUdpSocket = new QUdpSocket();
    mUdpSocket->bind(port);

    connect(mUdpSocket, SIGNAL(readyRead()), this, SLOT(readPendingNotifications()));

    qDebug() << "Listening to udp messages";

}

void AvahiAnnouncer::readPendingNotifications()
{

    qDebug() << "AvahiAnnouncer readPendingNotifications";
    
    while (mUdpSocket->hasPendingDatagrams()) {

        QByteArray datagram;
        datagram.resize(mUdpSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        mUdpSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        //log.write(datagram);
        qDebug() << "AvahiAnnouncer incomming udp datagram: " << datagram;

        NetworkPackage np;
        NetworkPackage::unserialize(datagram,&np);

        if (np.type() == "kdeconnect.identity") {

            QString id = np.get<QString>("deviceId");
            QString name = np.get<QString>("deviceName");

            qDebug() << "AvahiAnnouncer creating link to device" << id;

            DeviceLink* dl = new UdpDeviceLink(id, this, sender, 10600);
            links.append(dl);

            emit onNewDeviceLink(id, name, dl);
        } else {
            qDebug() << "Not an identification package (wuh?)";
        }
    }

}

AvahiAnnouncer::~AvahiAnnouncer()
{
    delete service;
}

void AvahiAnnouncer::setDiscoverable(bool b)
{
    qDebug() << "Avahi scanning";
    if (b) service->publishAsync();
}

