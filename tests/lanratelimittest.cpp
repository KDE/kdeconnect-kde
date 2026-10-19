/**
 * SPDX-FileCopyrightText: 2026 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QCoreApplication>
#include <QObject>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTcpSocket>
#include <QTest>

#include "core/backends/lan/lanlinkprovider.h"

class TestLanRateLimit : public QObject
{
    Q_OBJECT
public:
    TestLanRateLimit()
    {
        QStandardPaths::setTestModeEnabled(true);
    }

private Q_SLOTS:
    void testRateLimitTcpByIp()
    {
        LanLinkProvider provider(true);
        provider.onStart();
        QVERIFY(provider.tcpPort() != 0);

        // Let the rate-limit entry created by receiving our own UDP broadcast expire
        QTest::qWait(600);

        QTcpSocket first;
        QVERIFY(connectAndCheckAccepted(first, provider.tcpPort()));

        QTcpSocket second;
        QVERIFY(!connectAndCheckAccepted(second, provider.tcpPort()));

        QTest::qWait(600);

        QTcpSocket third;
        QVERIFY(connectAndCheckAccepted(third, provider.tcpPort()));

        provider.onStop();
    }

private:
    // Returns false if the server drops the connection right away
    bool connectAndCheckAccepted(QTcpSocket &socket, quint16 port)
    {
        QSignalSpy disconnectedSpy(&socket, &QAbstractSocket::disconnected);
        socket.connectToHost(QHostAddress::LocalHost, port);
        if (!socket.waitForConnected(1000)) {
            return false;
        }
        // The server waits 1s for an identity packet before closing an accepted connection
        return !disconnectedSpy.wait(300);
    }
};

QTEST_MAIN(TestLanRateLimit);

#include "lanratelimittest.moc"
