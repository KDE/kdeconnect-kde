/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "landevicelink.h"

#include <KLocalizedString>

#include "backends/linkprovider.h"
#include "core_debug.h"
#include "daemon.h"
#include "kdeconnectconfig.h"
#include "lanlinkprovider.h"
#include "plugins/share/shareplugin.h"

LanDeviceLink::LanDeviceLink(const DeviceInfo &deviceInfo, LanLinkProvider *linkProvider, QSslSocket *socket)
    : DeviceLink(linkProvider)
    , m_socket(nullptr)
    , m_deviceInfo(deviceInfo)
{
    reset(socket);
}

void LanDeviceLink::reset(QSslSocket *socket)
{
    if (m_socket) {
        delete m_socket;
    }

    m_socket = socket;
    socket->setParent(this);

    connect(socket, &QAbstractSocket::disconnected, this, &LanDeviceLink::disconnected);
    connect(socket, &QAbstractSocket::readyRead, this, &LanDeviceLink::dataReceived);
}

QHostAddress LanDeviceLink::hostAddress() const
{
    if (!m_socket) {
        return QHostAddress::Null;
    }
    QHostAddress addr = m_socket->peerAddress();
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

bool LanDeviceLink::sendPacket(NetworkPacket &np)
{
    if (np.payload() && np.hasPayload()) {
        Device *device = Daemon::instance()->getDevice(deviceId());
        if (device == nullptr) {
            qCWarning(KDECONNECT_CORE) << "Device disconnected" << deviceId();
            return false;
        }
        // FIXME: Remove packet-type-specific logic from the link
        if (np.type() == PACKET_TYPE_SHARE_REQUEST && np.payloadSize() >= 0) {
            if (!m_compositeUploadJob || !m_compositeUploadJob->isRunning()) {
                m_compositeUploadJob = new CompositeUploadJob(device, true);
            }

            m_compositeUploadJob->addSubjob(new UploadJob(np));

            if (!m_compositeUploadJob->isRunning()) {
                m_compositeUploadJob->start();
            }
        } else { // Infinite stream
            CompositeUploadJob *fireAndForgetJob = new CompositeUploadJob(device, false);
            fireAndForgetJob->addSubjob(new UploadJob(np));
            fireAndForgetJob->start();
        }

        return true;
    } else {
        int written = m_socket->write(np.serialize());

        // Actually we can't detect if a packet is received or not. We keep TCP
        //"ESTABLISHED" connections that look legit (return true when we use them),
        // but that are actually broken (until keepalive detects that they are down).
        return (written != -1);
    }
}

void LanDeviceLink::dataReceived()
{
    while (m_socket->canReadLine()) {
        const QByteArray serializedPacket = m_socket->readLine();
        NetworkPacket packet;
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

            const QString address = m_socket->peerAddress().toString();
            const quint16 port = transferInfo[QStringLiteral("port")].toInt();
            socket->connectToHostEncrypted(address, port, QIODevice::ReadWrite);
            packet.setPayload(socket, packet.payloadSize());
        }

        Q_EMIT receivedPacket(packet);
    }
}

#include "moc_landevicelink.cpp"
