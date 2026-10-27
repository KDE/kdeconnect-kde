/**
 * SPDX-FileCopyrightText: 2026 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QCoreApplication>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTest>
#include <QUdpSocket>

#include "core/backends/lan/lanlinkprovider.h"
#include "core/networkpacket.h"

class TestLanIPv6 : public QObject
{
    Q_OBJECT
public:
    TestLanIPv6()
    {
        QStandardPaths::setTestModeEnabled(true);
    }

private Q_SLOTS:
    void initTestCase()
    {
        QTcpServer probe;
        if (!probe.listen(QHostAddress::LocalHostIPv6)) {
            QSKIP("IPv6 loopback not available");
        }
    }

    // A peer connects to us over IPv6
    void testTcpConnectionIPv6()
    {
        LanLinkProvider provider(true);
        provider.testSetLoopbackAddress(QHostAddress::LocalHostIPv6);
        provider.onStart();
        QVERIFY(provider.tcpPort() != 0);

        QTcpSocket socket;
        QSignalSpy disconnectedSpy(&socket, &QAbstractSocket::disconnected);
        socket.connectToHost(QHostAddress::LocalHostIPv6, provider.tcpPort());
        QVERIFY(socket.waitForConnected(1000));
        // The server waits 1s for an identity packet before closing an accepted connection
        QVERIFY(!disconnectedSpy.wait(300));

        provider.onStop();
    }

    // A peer announces itself via UDP over IPv6 and we connect back to it
    void testUdpDiscoveryIPv6()
    {
        LanLinkProvider provider(true);
        provider.testSetLoopbackAddress(QHostAddress::LocalHostIPv6);
        provider.onStart();
        QVERIFY(provider.tcpPort() != 0);

        QTcpServer fakePeer;
        quint16 fakePeerPort = LanLinkProvider::MIN_TCP_PORT;
        while (!fakePeer.listen(QHostAddress::LocalHostIPv6, fakePeerPort)) {
            ++fakePeerPort;
            QVERIFY(fakePeerPort <= LanLinkProvider::MAX_TCP_PORT);
        }
        QSignalSpy newConnectionSpy(&fakePeer, &QTcpServer::newConnection);

        NetworkPacket np(PACKET_TYPE_IDENTITY);
        np.set(QStringLiteral("deviceId"), QStringLiteral("fake_peer_ipv6_0000000000000000000"));
        np.set(QStringLiteral("deviceName"), QStringLiteral("Fake peer"));
        np.set(QStringLiteral("protocolVersion"), NetworkPacket::s_protocolVersion);
        np.set(QStringLiteral("tcpPort"), fakePeerPort);

        QUdpSocket udpSocket;
        QVERIFY(udpSocket.writeDatagram(np.serialize(), QHostAddress::LocalHostIPv6, LanLinkProvider::UDP_PORT) > 0);

        QVERIFY(newConnectionSpy.wait(1000));
        QTcpSocket *incoming = fakePeer.nextPendingConnection();
        QVERIFY(incoming);
        QCOMPARE(incoming->peerAddress(), QHostAddress(QHostAddress::LocalHostIPv6));

        provider.onStop();
    }
};

QTEST_MAIN(TestLanIPv6);

#include "lanipv6test.moc"
