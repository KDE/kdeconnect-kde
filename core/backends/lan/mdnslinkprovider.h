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

#ifndef MDNSLINKPROVIDER_H
#define MDNSLINKPROVIDER_H

#include <QObject>
#include <QTcpServer>
#include <QSslSocket>
#include <QTimer>
#include <QNetworkSession>

#include "kdeconnectcore_export.h"
#include "backends/linkprovider.h"
#include "server.h"
#include "landevicelink.h"

namespace KDNSSD {
    class PublicService;
    class ServiceBrowser;
};

class LanPairingHandler;
class KDECONNECTCORE_EXPORT MDNSLinkProvider
    : public LinkProvider
{
    Q_OBJECT

public:
    MDNSLinkProvider();
    ~MDNSLinkProvider() override;

    QString name() override { return QStringLiteral("MDNSLinkProvider"); }
    int priority() override { return PRIORITY_HIGH; }

    void userRequestsPair(const QString& deviceId);
    void userRequestsUnpair(const QString& deviceId);
    void incomingPairPacket(DeviceLink* device, const NetworkPacket& np);

    static void configureSslSocket(QSslSocket* socket, const QString& deviceId, bool isDeviceTrusted);
    static void configureSocket(QSslSocket* socket);

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
    void newConnection();
    void dataReceived();
    void deviceLinkDestroyed(QObject* destroyedDeviceLink);
    void sslErrors(const QList<QSslError>& errors);

private:
    LanPairingHandler* createPairingHandler(DeviceLink* link);

    void onNetworkConfigurationChanged(const QNetworkConfiguration& config);
    void addLink(const QString& deviceId, QSslSocket* socket, NetworkPacket* receivedPacket, LanDeviceLink::ConnectionStarted connectionOrigin);
    void initializeMDNS();
    void sendOurIdentity(QSslSocket*);
    NetworkPacket* receiveTheirIdentity(QSslSocket*);

    Server* m_server;
    quint16 m_tcpPort;

    QMap<QString, LanDeviceLink*> m_links;
    QMap<QString, LanPairingHandler*> m_pairingHandlers;

    struct PendingConnect {
        NetworkPacket* np;
        QHostAddress sender;
    };
    QMap<QSslSocket*, PendingConnect> m_receivedIdentityPackets;
    QNetworkConfiguration m_lastConfig;

    KDNSSD::PublicService *m_publisher;
    KDNSSD::ServiceBrowser *m_serviceBrowser;
};

#endif
