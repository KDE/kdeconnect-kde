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

#include "avahitcplinkprovider.h"

#include "devicelinks/tcpdevicelink.h"

#include <QHostInfo>
#include <QTcpServer>

AvahiTcpLinkProvider::AvahiTcpLinkProvider()
{
    QString serviceType = "_kdeconnect._tcp";

    //http://api.kde.org/4.x-api/kdelibs-apidocs/dnssd/html/index.html

    service = new DNSSD::PublicService(QHostInfo::localHostName(), serviceType, port);

    mServer = new QTcpServer(this);
    connect(mServer,SIGNAL(newConnection()),this, SLOT(newConnection()));

}

void AvahiTcpLinkProvider::onStart()
{

    mServer->listen(QHostAddress::Any, port);
    service->publishAsync();
}

void AvahiTcpLinkProvider::onStop()
{
    mServer->close();
    service->stop();

}
void AvahiTcpLinkProvider::onNetworkChange(QNetworkSession::State state)
{
    Q_UNUSED(state);
    
    //Nothing to do, Avahi will handle it
}

void AvahiTcpLinkProvider::newConnection()
{
    qDebug() << "AvahiTcpLinkProvider newConnection";

    QTcpSocket* socket = mServer->nextPendingConnection();
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    connect(socket,SIGNAL(readyRead()),this,SLOT(dataReceived()));

    NetworkPackage np(PACKAGE_TYPE_IDENTITY);
    NetworkPackage::createIdentityPackage(&np);
    int written = socket->write(np.serialize());

    qDebug() << "AvahiTcpLinkProvider sent package." << written << " bytes written, waiting for reply";
}

void AvahiTcpLinkProvider::dataReceived()
{
    QTcpSocket* socket = (QTcpSocket*) QObject::sender();

    QByteArray data = socket->readLine();

    qDebug() << "AvahiTcpLinkProvider received reply:" << data;

    NetworkPackage np("");
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

        qDebug() << "AvahiTcpLinkProvider creating link to device" << id << "(" << socket->peerAddress() << ")";

        emit onNewDeviceLink(np, dl);

        disconnect(socket,SIGNAL(readyRead()),this,SLOT(dataReceived()));

    } else {
        qDebug() << "AvahiTcpLinkProvider/newConnection: Not an identification package (wuh?)";
    }

}

void AvahiTcpLinkProvider::deviceLinkDestroyed(QObject* deviceLink)
{
    const QString& id = ((DeviceLink*)deviceLink)->deviceId();
    if (links.contains(id)) links.remove(id);
}

AvahiTcpLinkProvider::~AvahiTcpLinkProvider()
{
    delete service;
}

