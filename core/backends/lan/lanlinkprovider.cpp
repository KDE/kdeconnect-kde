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
#include "core_debug.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include <QHostInfo>
#include <QTcpServer>
#include <QUdpSocket>
#include <QtGlobal>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>

#include "../../daemon.h"
#include "landevicelink.h"
#include "lanpairinghandler.h"
#include <kdeconnectconfig.h>
#include <QDBusPendingReply>
#include <QtNetwork/qsslcipher.h>
#include <QtNetwork/qsslconfiguration.h>

LanLinkProvider::LanLinkProvider()
{
    mTcpPort = 0;

    mUdpServer = new QUdpSocket(this);
    connect(mUdpServer, SIGNAL(readyRead()), this, SLOT(newUdpConnection()));

    mServer = new Server(this);
    connect(mServer,SIGNAL(newConnection()),this, SLOT(newConnection()));

    m_pairingHandler = new LanPairingHandler();

    //Detect when a network interface changes status, so we announce ourelves in the new network
    QNetworkConfigurationManager* networkManager;
    networkManager = new QNetworkConfigurationManager(this);
    connect(networkManager, &QNetworkConfigurationManager::configurationChanged, [this, networkManager](QNetworkConfiguration config) {
        Q_UNUSED(config);
        //qCDebug(KDECONNECT_CORE) << config.name() << " state changed to " << config.state();
        //qCDebug(KDECONNECT_CORE) << "Online status: " << (networkManager->isOnline()? "online":"offline");
        onNetworkChange();
    });
}

LanLinkProvider::~LanLinkProvider()
{

}

void LanLinkProvider::onStart()
{
    mUdpServer->bind(QHostAddress::Any, port, QUdpSocket::ShareAddress);

    mTcpPort = port;
    while (!mServer->listen(QHostAddress::Any, mTcpPort)) {
        mTcpPort++;
        if (mTcpPort > 1764) { //No ports available?
            qCritical(KDECONNECT_CORE) << "Error opening a port in range 1714-1764";
            mTcpPort = 0;
            return;
        }
    }

    onNetworkChange();
}

void LanLinkProvider::onStop()
{
    mUdpServer->close();
    mServer->close();
}

//I'm in a new network, let's be polite and introduce myself
void LanLinkProvider::onNetworkChange()
{
    if (!mServer->isListening()) {
        //Not started
        return;
    }

    Q_ASSERT(mTcpPort != 0);

    qCDebug(KDECONNECT_CORE()) << "Broadcasting identity packet";
    NetworkPackage np("");
    NetworkPackage::createIdentityPackage(&np);
    np.set("tcpPort", mTcpPort);
    mUdpSocket.writeDatagram(np.serialize(), QHostAddress("255.255.255.255"), port);
}

//I'm the existing device, a new device is kindly introducing itself.
//I will create a TcpSocket and try to connect. This can result in either connected() or connectError().
void LanLinkProvider::newUdpConnection()
{
    while (mUdpServer->hasPendingDatagrams()) {

        QByteArray datagram;
        datagram.resize(mUdpServer->pendingDatagramSize());
        QHostAddress sender;

        mUdpServer->readDatagram(datagram.data(), datagram.size(), &sender);

        NetworkPackage* receivedPackage = new NetworkPackage("");
        bool success = NetworkPackage::unserialize(datagram, receivedPackage);

        //qCDebug(KDECONNECT_CORE) << "Datagram " << datagram.data() ;

        if (!success || receivedPackage->type() != PACKAGE_TYPE_IDENTITY) {
            delete receivedPackage;
            continue;
        }

        if (receivedPackage->get<QString>("deviceId") == KdeConnectConfig::instance()->deviceId()) {
            //qCDebug(KDECONNECT_CORE) << "Ignoring my own broadcast";
            delete receivedPackage;
            continue;
        }

        int tcpPort = receivedPackage->get<int>("tcpPort", port);

        //qCDebug(KDECONNECT_CORE) << "Received Udp identity package from" << sender << " asking for a tcp connection on port " << tcpPort;

        QSslSocket* socket = new QSslSocket(this);
        receivedIdentityPackages[socket].np = receivedPackage;
        receivedIdentityPackages[socket].sender = sender;
        connect(socket, SIGNAL(connected()), this, SLOT(connected()));
        connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));
        socket->connectToHost(sender, tcpPort);
    }
}

void LanLinkProvider::connectError()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    disconnect(socket, SIGNAL(connected()), this, SLOT(connected()));
    disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));

    qCDebug(KDECONNECT_CORE) << "Fallback (1), try reverse connection (send udp packet)";
    NetworkPackage np("");
    NetworkPackage::createIdentityPackage(&np);
    np.set("tcpPort", mTcpPort);
    mUdpSocket.writeDatagram(np.serialize(), receivedIdentityPackages[socket].sender, port);

    //The socket we created didn't work, and we didn't manage
    //to create a LanDeviceLink from it, deleting everything.
    delete receivedIdentityPackages.take(socket).np;
    delete socket;
}

void LanLinkProvider::connected()
{
    qCDebug(KDECONNECT_CORE) << "Socket connected";

    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    disconnect(socket, SIGNAL(connected()), this, SLOT(connected()));
    disconnect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(connectError()));

    configureSocket(socket);

    // If socket disconnects due to any reason after connection, link on ssl faliure
    connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));

    NetworkPackage* receivedPackage = receivedIdentityPackages[socket].np;
    const QString& deviceId = receivedPackage->get<QString>("deviceId");
    //qCDebug(KDECONNECT_CORE) << "Connected" << socket->isWritable();

    // If network is on ssl, do not believe when they are connected, believe when handshake is completed

    NetworkPackage np2("");
    NetworkPackage::createIdentityPackage(&np2);
    socket->write(np2.serialize());
    bool success = socket->waitForBytesWritten();

    if (success) {

        qCDebug(KDECONNECT_CORE) << "Handshaking done (i'm the existing device)";

        // if ssl supported
        if (NetworkPackage::ProtocolVersion <= receivedPackage->get<int>("protocolVersion")) {
            // since I support ssl and remote device support ssl
	        qCDebug(KDECONNECT_CORE) << "Setting up ssl server";

            bool isDeviceTrusted = KdeConnectConfig::instance()->trustedDevices().contains(deviceId);

            socket->setPeerVerifyName(receivedPackage->get<QString>("deviceId"));

            if (isDeviceTrusted) {
                qDebug() << "Device trusted";
                QString certString = KdeConnectConfig::instance()->getTrustedDevice(deviceId).certificate;
                socket->addCaCertificate(QSslCertificate(certString.toLatin1()));
                socket->setPeerVerifyMode(QSslSocket::QueryPeer);
                connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
            } else {
                qDebug() << "Device untrusted";
                // Do not care about ssl errors here, socket will not be closed due to errors because of query peer
                socket->setPeerVerifyMode(QSslSocket::QueryPeer);
            }
            qCDebug(KDECONNECT_CORE) << "Starting server ssl";
            connect(socket, SIGNAL(encrypted()), this, SLOT(encrypted()));

            socket->startServerEncryption();
            return; // Return statement prevents from deleting received package, needed in slot "encrypted"
        } else {
            addLink(deviceId, socket, receivedPackage);
        }

    } else {
        //I think this will never happen, but if it happens the deviceLink
        //(or the socket that is now inside it) might not be valid. Delete them.
        qCDebug(KDECONNECT_CORE) << "Fallback (2), try reverse connection (send udp packet)";
        mUdpSocket.writeDatagram(np2.serialize(), receivedIdentityPackages[socket].sender, port);
    }

    delete receivedPackage;
    receivedIdentityPackages.remove(socket);
    //We don't delete the socket because now it's owned by the LanDeviceLink
}

void LanLinkProvider::encrypted()
{

    qCDebug(KDECONNECT_CORE) << "Socket encrypted";

    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    disconnect(socket, SIGNAL(encrypted()), this, SLOT(encrypted()));
    disconnect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));

    NetworkPackage* receivedPackage = receivedIdentityPackages[socket].np;
    const QString& deviceId = receivedPackage->get<QString>("deviceId");
    //qCDebug(KDECONNECT_CORE) << "Connected" << socket->isWritable();

    receivedPackage->set("certificate", socket->peerCertificate().toPem());

    addLink(deviceId, socket, receivedPackage);

    // Copied from connected slot, now delete received package
    delete receivedPackage;
    receivedIdentityPackages.remove(socket);

}

void LanLinkProvider::sslErrors(const QList<QSslError>& errors)
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());
    if (!socket) return;
    disconnect(socket, SIGNAL(encrypted()), this, SLOT(encrypted()));
    disconnect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));

    foreach(QSslError error, errors) {
        qCDebug(KDECONNECT_CORE) << "SSL Error :" << error.errorString();
        switch (error.error()) {
            case QSslError::CertificateSignatureFailed:
            case QSslError::CertificateNotYetValid:
            case QSslError::CertificateExpired:
            case QSslError::CertificateUntrusted:
            case QSslError::SelfSignedCertificate:
                qCDebug(KDECONNECT_CORE) << "Unpairing device due to " << error.errorString();
                // Not able to find an alternative now
                Daemon::instance()->getDevice(socket->peerVerifyName())->unpair();
                break;
            default:
                continue;
                // Lots of warnings without this

        }
    }

    delete receivedIdentityPackages.take(socket).np;
    // Socket disconnects itself on ssl error and will be deleted by deleteLater slot, no need to delete manually
}



//I'm the new device and this is the answer to my UDP identity package (no data received yet)
void LanLinkProvider::newConnection()
{
    //qCDebug(KDECONNECT_CORE) << "LanLinkProvider newConnection";

    while (mServer->hasPendingConnections()) {
        QSslSocket* socket = mServer->nextPendingConnection();
        configureSocket(socket);
        //This socket is still managed by us (and child of the QTcpServer), if
        //it disconnects before we manage to pass it to a LanDeviceLink, it's
        //our responsability to delete it. We do so with this connection.
        connect(socket, SIGNAL(disconnected()),
                socket, SLOT(deleteLater()));
        connect(socket, SIGNAL(readyRead()),
                this, SLOT(dataReceived()));

    }
}

//I'm the new device and this is the answer to my UDP identity package (data received)
void LanLinkProvider::dataReceived()
{
    QSslSocket* socket = qobject_cast<QSslSocket*>(sender());

    const QByteArray data = socket->readLine();

    qCDebug(KDECONNECT_CORE) << "LanLinkProvider received reply:" << data;

    NetworkPackage* np = new NetworkPackage("");
    bool success = NetworkPackage::unserialize(data, np);

    if (!success || np->type() != PACKAGE_TYPE_IDENTITY) {
        qCDebug(KDECONNECT_CORE) << "LanLinkProvider/newConnection: Not an identification package (wuh?)";
        return;
    }

    // Needed in "encrypted" if ssl is used, similar to "connected"
    receivedIdentityPackages[socket].np = np;

    const QString& deviceId = np->get<QString>("deviceId");
    //qCDebug(KDECONNECT_CORE) << "Handshaking done (i'm the new device)";

    //This socket will now be owned by the LanDeviceLink or we don't want more data to be received, forget about it
    disconnect(socket, SIGNAL(readyRead()), this, SLOT(dataReceived()));


    if (NetworkPackage::ProtocolVersion <= np->get<int>("protocolVersion")) {
        // since I support ssl and remote device support ssl
        qCDebug(KDECONNECT_CORE) << "Setting up ssl client";

        bool isDeviceTrusted = KdeConnectConfig::instance()->trustedDevices().contains(deviceId);

        socket->setPeerVerifyName(deviceId);

        if (isDeviceTrusted) {
            qDebug() << "Device trusted";
            QString certString = KdeConnectConfig::instance()->getTrustedDevice(deviceId).certificate;
	        socket->addCaCertificate(QSslCertificate(certString.toLatin1()));
            socket->setPeerVerifyMode(QSslSocket::VerifyPeer);
            connect(socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
        } else {
            qDebug() << "Device untrusted";
            // Do not care about ssl errors here, socket will not be closed due to errors because of query peer
            socket->setPeerVerifyMode(QSslSocket::QueryPeer);
        }
        qCDebug(KDECONNECT_CORE) << "Starting client ssl";
        connect(socket, SIGNAL(encrypted()), this, SLOT(encrypted()));

        socket->startClientEncryption();
        return;
    } else {
        addLink(deviceId, socket, np);
    }

    delete np;
    receivedIdentityPackages.remove(socket);

}

void LanLinkProvider::deviceLinkDestroyed(QObject* destroyedDeviceLink)
{
    //qCDebug(KDECONNECT_CORE) << "deviceLinkDestroyed";
    const QString id = destroyedDeviceLink->property("deviceId").toString();
    QMap< QString, DeviceLink* >::iterator oldLinkIterator = mLinks.find(id);
    if (oldLinkIterator != mLinks.end() && oldLinkIterator.value() == destroyedDeviceLink) {
        mLinks.erase(oldLinkIterator);
    }

}

void LanLinkProvider::configureSocket(QSslSocket* socket)
{
    int fd = socket->socketDescriptor();

    socket->setSocketOption(QAbstractSocket::KeepAliveOption, QVariant(1));

    // Configure for ssl
    socket->setLocalCertificate(KdeConnectConfig::instance()->certificate());
    socket->setPrivateKey(KdeConnectConfig::instance()->privateKeyPath());
    socket->setProtocol(QSsl::TlsV1_2);

    #ifdef TCP_KEEPIDLE
        // time to start sending keepalive packets (seconds)
        int maxIdle = 10;
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPIDLE, &maxIdle, sizeof(maxIdle));
    #endif

    #ifdef TCP_KEEPINTVL
        // interval between keepalive packets after the initial period (seconds)
        int interval = 5;
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPINTVL, &interval, sizeof(interval));
    #endif

    #ifdef TCP_KEEPCNT
        // number of missed keepalive packets before disconnecting
        int count = 3;
        setsockopt(fd, IPPROTO_TCP, TCP_KEEPCNT, &count, sizeof(count));
    #endif

}

void LanLinkProvider::addLink(QString deviceId, QSslSocket* socket, NetworkPackage* receivedPackage) {

    LanDeviceLink* deviceLink = new LanDeviceLink(deviceId, this, socket);
    connect(deviceLink, SIGNAL(destroyed(QObject*)), this, SLOT(deviceLinkDestroyed(QObject*)));

    // Socket disconnection will now be handled by LanDeviceLink
    disconnect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));

    if (socket->isEncrypted()) {
        deviceLink->setOnSsl(true);
    }

    //We kill any possible link from this same device
    QMap< QString, DeviceLink* >::iterator oldLinkIterator = mLinks.find(deviceLink->deviceId());
    if (oldLinkIterator != mLinks.end()) {
        DeviceLink* oldLink = oldLinkIterator.value();
        disconnect(oldLink, SIGNAL(destroyed(QObject*)),
                   this, SLOT(deviceLinkDestroyed(QObject*)));
        oldLink->deleteLater();
        mLinks.erase(oldLinkIterator);
    }

    mLinks[deviceLink->deviceId()] = deviceLink;

    Q_EMIT onConnectionReceived(*receivedPackage, deviceLink);

}