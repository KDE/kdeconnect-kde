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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LANLINKPROVIDER_H
#define LANLINKPROVIDER_H

#include <QObject>
#include <QTcpServer>
#include <QSslSocket>
#include <QUdpSocket>
#include <QTimer>
#include <QNetworkSession>

#include "kdeconnectcore_export.h"
#include "backends/linkprovider.h"
#include "server.h"
#include "landevicelink.h"

class LanPairingHandler;
class KDECONNECTCORE_EXPORT LanLinkProvider
    : public LinkProvider
{
    Q_OBJECT

public:
    /**
     * @param testMode Some special overrides needed while testing
     * @param udpBroadcastPort Port which should be used for *sending* identity packets
     * @param udpListenPort Port which should be used for *receiving* identity packets
     */
    LanLinkProvider(
            bool testMode = false,
            quint16 udpBroadcastPort = UDP_PORT,
            quint16 udpListenPort = UDP_PORT
            );
    ~LanLinkProvider() override;

    QString name() override { return QStringLiteral("LanLinkProvider"); }
    int priority() override { return PRIORITY_HIGH; }

    void userRequestsPair(const QString& deviceId);
    void userRequestsUnpair(const QString& deviceId);
    void incomingPairPacket(DeviceLink* device, const NetworkPacket& np);

    static void configureSslSocket(QSslSocket* socket, const QString& deviceId, bool isDeviceTrusted);
    static void configureSocket(QSslSocket* socket);

    /**
     * This is the default UDP port both for broadcasting and receiving identity packets
     */
    const static quint16 UDP_PORT = 1716;
    const static quint16 MIN_TCP_PORT = 1716;
    const static quint16 MAX_TCP_PORT = 1764;

public Q_SLOTS:
    void onNetworkChange() override;
    void onStart() override;
    void onStop() override;
    void tcpSocketConnected();
    void encrypted();
    void connectError(QAbstractSocket::SocketError socketError);

private Q_SLOTS:
    void udpBroadcastReceived();
    void newConnection();
    void dataReceived();
    void deviceLinkDestroyed(QObject* destroyedDeviceLink);
    void sslErrors(const QList<QSslError>& errors);
    void broadcastToNetwork();

private:
    LanPairingHandler* createPairingHandler(DeviceLink* link);

    void onNetworkConfigurationChanged(const QNetworkConfiguration& config);
    void addLink(const QString& deviceId, QSslSocket* socket, NetworkPacket* receivedPacket, LanDeviceLink::ConnectionStarted connectionOrigin);

    Server* m_server;
    QUdpSocket m_udpSocket;
    quint16 m_tcpPort;

    quint16 m_udpBroadcastPort;
    quint16 m_udpListenPort;

    QMap<QString, LanDeviceLink*> m_links;
    QMap<QString, LanPairingHandler*> m_pairingHandlers;

    struct PendingConnect {
        NetworkPacket* np;
        QHostAddress sender;
    };
    QMap<QSslSocket*, PendingConnect> m_receivedIdentityPackets;
    QNetworkConfiguration m_lastConfig;
    const bool m_testMode;
    QTimer m_combineBroadcastsTimer;
};

#endif
