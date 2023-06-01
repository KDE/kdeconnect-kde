/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "landevicelink.h"

#include <KLocalizedString>

#include "backends/linkprovider.h"
#include "core_debug.h"
#include "kdeconnectconfig.h"
#include "lanlinkprovider.h"
#include "plugins/share/shareplugin.h"
#include "socketlinereader.h"

LanDeviceLink::LanDeviceLink(const QString &deviceId, LinkProvider *parent, QSslSocket *socket, ConnectionStarted connectionSource)
    : DeviceLink(deviceId, parent)
    , m_socketLineReader(nullptr)
{
    reset(socket, connectionSource);
}

void LanDeviceLink::reset(QSslSocket *socket, ConnectionStarted connectionSource)
{
    if (m_socketLineReader) {
        disconnect(m_socketLineReader->m_socket, &QAbstractSocket::disconnected, this, &QObject::deleteLater);
        delete m_socketLineReader;
    }

    m_socketLineReader = new SocketLineReader(socket, this);

    connect(socket, &QAbstractSocket::disconnected, this, &QObject::deleteLater);
    connect(m_socketLineReader, &SocketLineReader::readyRead, this, &LanDeviceLink::dataReceived);

    // We take ownership of the socket.
    // When the link provider destroys us,
    // the socket (and the reader) will be
    // destroyed as well
    socket->setParent(m_socketLineReader);

    m_connectionSource = connectionSource;

    QString certString = KdeConnectConfig::instance().getDeviceProperty(deviceId(), QStringLiteral("certificate"));
}

QHostAddress LanDeviceLink::hostAddress() const
{
    if (!m_socketLineReader) {
        return QHostAddress::Null;
    }
    QHostAddress addr = m_socketLineReader->m_socket->peerAddress();
    if (addr.protocol() == QAbstractSocket::IPv6Protocol) {
        bool success;
        QHostAddress convertedAddr = QHostAddress(addr.toIPv4Address(&success));
        if (success) {
            qCDebug(KDECONNECT_CORE) << "Converting IPv6" << addr << "to IPv4" << convertedAddr;
            addr = convertedAddr;
        }
    }
    return addr;
}

QString LanDeviceLink::name()
{
    return QStringLiteral("LanLink"); // Should be same in both android and kde version
}

bool LanDeviceLink::sendPacket(NetworkPacket &np)
{
    if (np.payload()) {
        if (np.type() == PACKET_TYPE_SHARE_REQUEST && np.payloadSize() >= 0) {
            if (!m_compositeUploadJob || !m_compositeUploadJob->isRunning()) {
                m_compositeUploadJob = new CompositeUploadJob(deviceId(), true);
            }

            m_compositeUploadJob->addSubjob(new UploadJob(np));

            if (!m_compositeUploadJob->isRunning()) {
                m_compositeUploadJob->start();
            }
        } else { // Infinite stream
            CompositeUploadJob *fireAndForgetJob = new CompositeUploadJob(deviceId(), false);
            fireAndForgetJob->addSubjob(new UploadJob(np));
            fireAndForgetJob->start();
        }

        return true;
    } else {
        int written = m_socketLineReader->write(np.serialize());

        // Actually we can't detect if a packet is received or not. We keep TCP
        //"ESTABLISHED" connections that look legit (return true when we use them),
        // but that are actually broken (until keepalive detects that they are down).
        return (written != -1);
    }
}

void LanDeviceLink::dataReceived()
{
    if (!m_socketLineReader->hasPacketsAvailable())
        return;

    const QByteArray serializedPacket = m_socketLineReader->readLine();
    NetworkPacket packet((QString()));
    NetworkPacket::unserialize(serializedPacket, &packet);

    // qCDebug(KDECONNECT_CORE) << "LanDeviceLink dataReceived" << serializedPacket;

    if (packet.hasPayloadTransferInfo()) {
        // qCDebug(KDECONNECT_CORE) << "HasPayloadTransferInfo";
        const QVariantMap transferInfo = packet.payloadTransferInfo();

        QSharedPointer<QSslSocket> socket(new QSslSocket);

        LanLinkProvider::configureSslSocket(socket.data(), deviceId(), true);

        // emit readChannelFinished when the socket gets disconnected. This seems to be a bug in upstream QSslSocket.
        // Needs investigation and upstreaming of the fix. QTBUG-62257
        connect(socket.data(), &QAbstractSocket::disconnected, socket.data(), &QAbstractSocket::readChannelFinished);

        const QString address = m_socketLineReader->peerAddress().toString();
        const quint16 port = transferInfo[QStringLiteral("port")].toInt();
        socket->connectToHostEncrypted(address, port, QIODevice::ReadWrite);
        packet.setPayload(socket, packet.payloadSize());
    }

    Q_EMIT receivedPacket(packet);

    if (m_socketLineReader->hasPacketsAvailable()) {
        QMetaObject::invokeMethod(this, "dataReceived", Qt::QueuedConnection);
    }
}

QSslCertificate LanDeviceLink::certificate() const
{
    return m_socketLineReader->peerCertificate();
}
