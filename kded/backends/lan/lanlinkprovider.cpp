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

#include "lanlinkprovider.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <QHostInfo>
#include <QTcpServer>
#include <QUdpSocket>

#include "../../kdebugnamespace.h"
#include "landevicelink.h"

void LanLinkProvider::configureSocket(QTcpSocket* socket)
{
    int fd = socket->socketDescriptor();
    char enableKeepAlive = 1;
    setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &enableKeepAlive, sizeof(enableKeepAlive));

#ifdef TCP_KEEPIDLE
    int maxIdle = 60; /* seconds */
    setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &maxIdle, sizeof(maxIdle));
#endif

#ifdef TCP_KEEPCNT
    int count = 3;  // send up to 3 keepalive packets out, then disconnect if no response
    setsockopt(fd, getprotobyname("TCP")->p_proto, TCP_KEEPCNT, &count, sizeof(count));
#endif

#ifdef TCP_KEEPINTVL
    int interval = 5;   // send a keepalive packet out every 2 seconds (after the 5 second idle period)
    setsockopt(fd, getprotobyname("TCP")->p_proto, TCP_KEEPINTVL, &interval, sizeof(interval));
#endif

}

LanLinkProvider::LanLinkProvider()
{

    mUdpServer = new QUdpSocket(this);
    connect(mUdpServer, SIGNAL(readyRead()), this, SLOT(newUdpConnection()));

    mTcpServer = new QTcpServer(this);
    connect(mTcpServer,SIGNAL(newConnection()),this, SLOT(newConnection()));

}

void LanLinkProvider::onStart()
{
    mUdpServer->bind(QHostAddress::Broadcast, port, QUdpSocket::ShareAddress);

    tcpPort = port;
    while (!mTcpServer->listen(QHostAddress::Any, tcpPort)) tcpPort++;

    onNetworkChange(QNetworkSession::Connected);
}

void LanLinkProvider::onStop()
{
    mUdpServer->close();
    mTcpServer->close();
}

//I'm in a new network, let's be polite and introduce myself
void LanLinkProvider::onNetworkChange(QNetworkSession::State state)
{
    Q_UNUSED(state);

    if (!mTcpServer->isListening()) return;

    NetworkPackage np("");
    NetworkPackage::createIdentityPackage(&np);
    np.set("tcpPort",tcpPort);
    QUdpSocket().writeDatagram(np.serialize(),QHostAddress("255.255.255.255"), port);
}

//I'm the existing device, a new device is kindly introducing itself (I will create a TcpSocket)
void LanLinkProvider::newUdpConnection()
{
    while (mUdpServer->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(mUdpServer->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        mUdpServer->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        NetworkPackage* np = new NetworkPackage("");
        bool success = NetworkPackage::unserialize(datagram,np);

        if (success && np->type() == PACKAGE_TYPE_IDENTITY) {

            NetworkPackage np2("");
            NetworkPackage::createIdentityPackage(&np2);

            if (np->get<QString>("deviceId") == np2.get<QString>("deviceId")) {
                //kDebug(kdeconnect_kded()) << "Ignoring my own broadcast";
                return;
            }

            int tcpPort = np->get<int>("tcpPort",port);

            //kDebug(kdeconnect_kded()) << "Received Udp presentation from" << sender << "asking for a tcp connection on port " << tcpPort;

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

void LanLinkProvider::connectError()
{
    QTcpSocket* socket = (QTcpSocket*)sender();

    disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));
    disconnect(socket, SIGNAL(connected()), this, SLOT(connected()));

    kDebug(kdeconnect_kded()) << "Fallback (1), try reverse connection";
    NetworkPackage np("");
    NetworkPackage::createIdentityPackage(&np);
    np.set("tcpPort",tcpPort);
    QUdpSocket().writeDatagram(np.serialize(), receivedIdentityPackages[socket].sender, port);

}

void LanLinkProvider::connected()
{

    QTcpSocket* socket = (QTcpSocket*)sender();

    disconnect(socket, SIGNAL(connected()), this, SLOT(connected()));
    disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));

    configureSocket(socket);

    NetworkPackage* np = receivedIdentityPackages[socket].np;
    const QString& id = np->get<QString>("deviceId");
    //kDebug(kdeconnect_kded()) << "Connected" << socket->isWritable();

    LanDeviceLink* dl = new LanDeviceLink(id, this, socket);

    NetworkPackage np2("");
    NetworkPackage::createIdentityPackage(&np2);

    bool success = dl->sendPackage(np2);

    //TODO: Use reverse connection too to preffer connecting a unstable device (a phone) to a stable device (a computer)
    if (success) {

        //kDebug(kdeconnect_kded()) << "Handshaking done (i'm the existing device)";

        connect(dl, SIGNAL(destroyed(QObject*)),
                this, SLOT(deviceLinkDestroyed(QObject*)));

        Q_EMIT onConnectionReceived(*np, dl);

        QMap< QString, DeviceLink* >::iterator oldLinkIterator = links.find(id);
        if (oldLinkIterator != links.end()) {
            DeviceLink* oldLink = oldLinkIterator.value();
            disconnect(oldLink, SIGNAL(destroyed(QObject*)),
                        this, SLOT(deviceLinkDestroyed(QObject*)));
            oldLink->deleteLater();
            links.erase(oldLinkIterator);
        }

        links[id] = dl;

    } else {
        //I think this will never happen
        kDebug(kdeconnect_kded()) << "Fallback (2), try reverse connection";
        QUdpSocket().writeDatagram(np2.serialize(), receivedIdentityPackages[socket].sender, port);
        delete dl;
    }

    receivedIdentityPackages.remove(socket);

    delete np;

}

//I'm the new device and this is the answer to my UDP introduction (no data received yet)
void LanLinkProvider::newConnection()
{
    //kDebug(kdeconnect_kded()) << "LanLinkProvider newConnection";

    QTcpSocket* socket = mTcpServer->nextPendingConnection();
    socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);

    connect(socket,SIGNAL(readyRead()),this,SLOT(dataReceived()));

/*
    NetworkPackage np(PACKAGE_TYPE_IDENTITY);
    NetworkPackage::createIdentityPackage(&np);
    int written = socket->write(np.serialize());

    kDebug(kdeconnect_kded()) << "LanLinkProvider sent package." << written << " bytes written, waiting for reply";
*/
}

//I'm the new device and this is the answer to my UDP introduction (data received)
void LanLinkProvider::dataReceived()
{
    QTcpSocket* socket = (QTcpSocket*) QObject::sender();
    configureSocket(socket);

    QByteArray data = socket->readLine();

    //kDebug(kdeconnect_kded()) << "LanLinkProvider received reply:" << data;

    NetworkPackage np("");
    bool success = NetworkPackage::unserialize(data,&np);

    if (success && np.type() == PACKAGE_TYPE_IDENTITY) {

        const QString& id = np.get<QString>("deviceId");
        LanDeviceLink* dl = new LanDeviceLink(id, this, socket);

        //kDebug(kdeconnect_kded()) << "Handshaking done (i'm the new device)";

        connect(dl, SIGNAL(destroyed(QObject*)),
                this, SLOT(deviceLinkDestroyed(QObject*)));

        Q_EMIT onConnectionReceived(np, dl);

        QMap< QString, DeviceLink* >::iterator oldLinkIterator = links.find(id);
        if (oldLinkIterator != links.end()) {
            DeviceLink* oldLink = oldLinkIterator.value();
            disconnect(oldLink, SIGNAL(destroyed(QObject*)),
                        this, SLOT(deviceLinkDestroyed(QObject*)));
            oldLink->deleteLater();
            links.erase(oldLinkIterator);
        }

        links[id] = dl;

        disconnect(socket,SIGNAL(readyRead()),this,SLOT(dataReceived()));

    } else {
        kDebug(kdeconnect_kded()) << "LanLinkProvider/newConnection: Not an identification package (wuh?)";
    }

}

void LanLinkProvider::deviceLinkDestroyed(QObject* uncastedDeviceLink)
{
    //kDebug(kdeconnect_kded()) << "deviceLinkDestroyed";

    DeviceLink* deviceLink = (DeviceLink*)uncastedDeviceLink;
    const QString& id = deviceLink->deviceId();

    QMap< QString, DeviceLink* >::iterator oldLinkIterator = links.find(id);
    if (oldLinkIterator != links.end() && oldLinkIterator.value() == deviceLink) {
        links.erase(oldLinkIterator);
    }

}

LanLinkProvider::~LanLinkProvider()
{

}
