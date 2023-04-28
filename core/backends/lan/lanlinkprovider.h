/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef LANLINKPROVIDER_H
#define LANLINKPROVIDER_H

#include <QObject>
#include <QSslSocket>
#include <QTcpServer>
#include <QTimer>
#include <QUdpSocket>
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QNetworkConfigurationManager>
#include <QNetworkSession>
#else
#include <QNetworkInformation>
#endif

#include "backends/linkprovider.h"
#include "kdeconnectcore_export.h"
#include "landevicelink.h"
#include "server.h"

class LanPairingHandler;
class KDECONNECTCORE_EXPORT LanLinkProvider : public LinkProvider
{
    Q_OBJECT

public:
    /**
     * @param testMode Some special overrides needed while testing
     * @param udpBroadcastPort Port which should be used for *sending* identity packets
     * @param udpListenPort Port which should be used for *receiving* identity packets
     */
    LanLinkProvider(bool testMode = false, quint16 udpBroadcastPort = UDP_PORT, quint16 udpListenPort = UDP_PORT);
    ~LanLinkProvider() override;

    QString name() override
    {
        return QStringLiteral("LanLinkProvider");
    }
    int priority() override
    {
        return PRIORITY_HIGH;
    }

    void userRequestsPair(const QString &deviceId);
    void userRequestsUnpair(const QString &deviceId);
    void incomingPairPacket(DeviceLink *device, const NetworkPacket &np);

    static void configureSslSocket(QSslSocket *socket, const QString &deviceId, bool isDeviceTrusted);
    static void configureSocket(QSslSocket *socket);

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
    void deviceLinkDestroyed(QObject *destroyedDeviceLink);
    void sslErrors(const QList<QSslError> &errors);
    void broadcastToNetwork();

private:
    LanPairingHandler *createPairingHandler(DeviceLink *link);

    void addLink(const QString &deviceId, QSslSocket *socket, NetworkPacket *receivedPacket, LanDeviceLink::ConnectionStarted connectionOrigin);
    QList<QHostAddress> getBroadcastAddresses();
    void sendBroadcasts(QUdpSocket &socket, const NetworkPacket &np, const QList<QHostAddress> &addresses);

    Server *m_server;
    QUdpSocket m_udpSocket;
    quint16 m_tcpPort;

    quint16 m_udpBroadcastPort;
    quint16 m_udpListenPort;

    QMap<QString, LanDeviceLink *> m_links;
    QMap<QString, LanPairingHandler *> m_pairingHandlers;

    struct PendingConnect {
        NetworkPacket *np;
        QHostAddress sender;
    };
    QMap<QSslSocket *, PendingConnect> m_receivedIdentityPackets;
    const bool m_testMode;
    QTimer m_combineBroadcastsTimer;
#if QT_VERSION_MAJOR < 6
    QNetworkConfiguration m_lastConfig;
#endif
};

#endif
