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

#include "broadcasttcplinkprovider.h"

#include "devicelinks/tcpdevicelink.h"

#include <QHostInfo>
#include <QTcpServer>
#include <QUdpSocket>

BroadcastTcpLinkProvider::BroadcastTcpLinkProvider()
{

    mUdpServer = new QUdpSocket(this);
    connect(mUdpServer, SIGNAL(readyRead()), this, SLOT(newUdpConnection()));

    mTcpServer = new QTcpServer(this);
    connect(mTcpServer,SIGNAL(newConnection()),this, SLOT(newConnection()));

}

void BroadcastTcpLinkProvider::onStart()
{
    mUdpServer->bind(QHostAddress::Broadcast, port, QUdpSocket::ShareAddress);

    tcpPort = port;
    while (!mTcpServer->listen(QHostAddress::Any, tcpPort)) tcpPort++;

    onNetworkChange(QNetworkSession::Connected);
}

void BroadcastTcpLinkProvider::onStop()
{
    mUdpServer->close();
    mTcpServer->close();
}

//I'm in a new network, let's be polite and introduce myself
void BroadcastTcpLinkProvider::onNetworkChange(QNetworkSession::State state)
{
    qDebug() << "onNetworkChange" << state;
    NetworkPackage np("");
    NetworkPackage::createIdentityPackage(&np);
    np.set("tcpPort",tcpPort);
    QUdpSocket().writeDatagram(np.serialize(),QHostAddress("255.255.255.255"), port);
}

//I'm the existing device, a new device is kindly introducing itself (I will create a TcpSocket)
void BroadcastTcpLinkProvider::newUdpConnection()
{
    while (mUdpServer->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(mUdpServer->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        mUdpServer->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        NetworkPackage* np = new NetworkPackage("");
        NetworkPackage::unserialize(datagram,np);

        if (np->version() > 0 && np->type() == PACKAGE_TYPE_IDENTITY) {

            NetworkPackage np2("");
            NetworkPackage::createIdentityPackage(&np2);

            if (np->get<QString>("deviceId") == np2.get<QString>("deviceId")) {
                qDebug() << "Ignoring my own broadcast";
                return;
            }

            const QString& id = np->get<QString>("deviceId");
            if (links.contains(id)) {
                //Delete old link if we already know it, probably it is down if this happens.
                qDebug() << "Destroying old link";
                delete links[id];
                links.remove(id);
            }

            int tcpPort = np->get<int>("tcpPort",port);

            qDebug() << "Received Udp presentation from" << sender << "asking for a tcp connection on port " << tcpPort;

            QTcpSocket* socket = new QTcpSocket(this);
            receivedIdentityPackages[socket].np = np;
            receivedIdentityPackages[socket].sender = sender;
            connect(socket, SIGNAL(connected()), this, SLOT(connected()));
            connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));
            socket->connectToHost(sender, tcpPort);

        } else {

            delete np;

        }
    }

}

void BroadcastTcpLinkProvider::connectError()
{
    QTcpSocket* socket = (QTcpSocket*)sender();

    disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));
    disconnect(socket, SIGNAL(connected()), this, SLOT(connected()));

    qDebug() << "Fallback (1), try reverse connection";
    NetworkPackage np("");
    NetworkPackage::createIdentityPackage(&np);
    np.set("tcpPort",tcpPort);
    QUdpSocket().writeDatagram(np.serialize(), receivedIdentityPackages[socket].sender, port);

}

void BroadcastTcpLinkProvider::connected()
{

    QTcpSocket* socket = (QTcpSocket*)sender();

    disconnect(socket, SIGNAL(connected()), this, SLOT(connected()));
    disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));

    NetworkPackage* np = receivedIdentityPackages[socket].np;
    const QString& id = np->get<QString>("deviceId");
    qDebug() << "Connected" << socket->isWritable();

    TcpDeviceLink* dl = new TcpDeviceLink(id, this, socket);

    connect(dl,SIGNAL(destroyed(QObject*)),this,SLOT(deviceLinkDestroyed(QObject*)));

    links[id] = dl;

    NetworkPackage np2("");
    NetworkPackage::createIdentityPackage(&np2);

    bool success = dl->sendPackage(np2);

    //TODO: Use reverse connection too to preffer connecting a unstable device (a phone) to a stable device (a computer)
    if (success) {
        qDebug() << "Handshaking done (i'm the existing device)";
        emit onNewDeviceLink(*np, dl);
    } else {
        //I think this will never happen
        qDebug() << "Fallback (2), try reverse connection";
        QUdpSocket().writeDatagram(np2.serialize(), receivedIdentityPackages[socket].sender, port);
        delete dl;
    }

    receivedIdentityPackages.remove(socket);
    delete np;

}

//I'm the new device and this is the answer to my UDP introduction (no data received yet)
void BroadcastTcpLinkProvider::newConnection()
{
    qDebug() << "BroadcastTcpLinkProvider newConnection";

    QTcpSocket* socket = mTcpServer->nextPendingConnection();
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    connect(socket,SIGNAL(readyRead()),this,SLOT(dataReceived()));

/*
    NetworkPackage np(PACKAGE_TYPE_IDENTITY);
    NetworkPackage::createIdentityPackage(&np);
    int written = socket->write(np.serialize());

    qDebug() << "BroadcastTcpLinkProvider sent package." << written << " bytes written, waiting for reply";
*/
}

//I'm the new device and this is the answer to my UDP introduction (data received)
void BroadcastTcpLinkProvider::dataReceived()
{
    QTcpSocket* socket = (QTcpSocket*) QObject::sender();

    QByteArray data = socket->readLine();

    qDebug() << "BroadcastTcpLinkProvider received reply:" << data;

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

        //qDebug() << "BroadcastTcpLinkProvider creating link to device" << id << "(" << socket->peerAddress() << ")";

        qDebug() << "Handshaking done (i'm the new device)";

        emit onNewDeviceLink(np, dl);

        disconnect(socket,SIGNAL(readyRead()),this,SLOT(dataReceived()));

    } else {
        qDebug() << "BroadcastTcpLinkProvider/newConnection: Not an identification package (wuh?)";
    }

}

void BroadcastTcpLinkProvider::deviceLinkDestroyed(QObject* deviceLink)
{
    const QString& id = ((DeviceLink*)deviceLink)->deviceId();
    qDebug() << "deviceLinkDestroyed";
    if (links.contains(id)) {
        qDebug() << "removing link from link list";
        links.remove(id);
    }
}

BroadcastTcpLinkProvider::~BroadcastTcpLinkProvider()
{

}
