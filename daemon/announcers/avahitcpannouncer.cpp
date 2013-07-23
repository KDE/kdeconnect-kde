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

#include "avahitcpannouncer.h"

#include "devicelinks/tcpdevicelink.h"

#include <QHostInfo>
#include <QTcpServer>

AvahiTcpAnnouncer::AvahiTcpAnnouncer()
{
    QString serviceType = "_kdeconnect._tcp";
    quint16 port = 10602;

    //http://api.kde.org/4.x-api/kdelibs-apidocs/dnssd/html/index.html

    service = new DNSSD::PublicService(QHostInfo::localHostName(), serviceType, port);

    mServer = new QTcpServer(this);
    connect(mServer,SIGNAL(newConnection()),this, SLOT(newConnection()));
    mServer->listen(QHostAddress::Any, port);

}

void AvahiTcpAnnouncer::newConnection()
{
    qDebug() << "AvahiTcpAnnouncer newConnection";

    QTcpSocket* socket = mServer->nextPendingConnection();
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    connect(socket,SIGNAL(readyRead()),this,SLOT(dataReceived()));

    NetworkPackage np;
    NetworkPackage::createIdentityPackage(&np);
    qDebug() << socket->isWritable();
    qDebug() << socket->isOpen();
    int written = socket->write(np.serialize());

    qDebug() << np.serialize();
    qDebug() << "AvahiTcpAnnouncer sent tcp package" << written << " bytes written, waiting for reply";
}

void AvahiTcpAnnouncer::dataReceived()
{
    QTcpSocket* socket = (QTcpSocket*) QObject::sender();

    QByteArray data = socket->readLine();

    qDebug() << "AvahiTcpAnnouncer received reply:" << data;

    NetworkPackage np;
    NetworkPackage::unserialize(data,&np);

    if (np.version() > 0 && np.type() == PACKAGE_TYPE_IDENTITY) {

        const QString& id = np.get<QString>("deviceId");
        TcpDeviceLink* dl = new TcpDeviceLink(id, this, socket);

        connect(dl,SIGNAL(destroyed(QObject*)),this,SLOT(deviceLinkDestroyed(QObject*)));

        if (links.contains(id)) {
            //Delete old link if we already know it, probably it is down if this happens.
            qDebug() << "Destroying old link";
            delete links[id];
        }
        links[id] = dl;

        qDebug() << "AvahiAnnouncer creating link to device" << id << "(" << socket->peerAddress() << ")";

        emit onNewDeviceLink(np, dl);

        disconnect(socket,SIGNAL(readyRead()),this,SLOT(dataReceived()));

    } else {
        qDebug() << "AvahiTcpAnnouncer/newConnection: Not an identification package (wuh?)";
    }

}

void AvahiTcpAnnouncer::deviceLinkDestroyed(QObject* deviceLink)
{
    const QString& id = ((DeviceLink*)deviceLink)->deviceId();
    if (links.contains(id)) links.remove(id);
}

AvahiTcpAnnouncer::~AvahiTcpAnnouncer()
{
    delete service;
}

void AvahiTcpAnnouncer::setDiscoverable(bool b)
{
    qDebug() << "AvahiTcp announcing";
    if (b) service->publishAsync();
}

