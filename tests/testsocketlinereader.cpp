/*************************************************************************************
 *  SPDX-FileCopyrightText: 2014 Alejandro Fiestas Olivares <afiestas@kde.org>       *
 *                                                                                   *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *************************************************************************************/

#include "../core/backends/lan/socketlinereader.h"
#include "../core/backends/lan/server.h"
#include "../core/qtcompat_p.h"

#include <QTest>
#include <QSslSocket>
#include <QProcess>
#include <QEventLoop>
#include <QTimer>

class TestSocketLineReader : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void initTestCase();
    void newPacket();

private Q_SLOTS:
    void socketLineReader();

private:
    QTimer m_timer;
    QEventLoop m_loop;
    QList<QByteArray> m_packets;
    Server* m_server;
    QSslSocket* m_conn;
    SocketLineReader* m_reader;
};

void TestSocketLineReader::initTestCase()
{
    m_server = new Server(this);

    QVERIFY2(m_server->listen(QHostAddress::LocalHost, 8694), "Failed to create local tcp server");

    m_timer.setInterval(4000);//For second is more enough to send some data via local socket
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, &m_loop, &QEventLoop::quit);

    m_conn = new QSslSocket(this);
    m_conn->connectToHost(QHostAddress::LocalHost, 8694);
    connect(m_conn, &QAbstractSocket::connected, &m_loop, &QEventLoop::quit);
    m_timer.start();
    m_loop.exec();

    QVERIFY2(m_conn->isOpen(), "Could not connect to local tcp server");
}

void TestSocketLineReader::socketLineReader()
{
    QList<QByteArray> dataToSend;
    dataToSend << "foobar\n" << "barfoo\n" << "foobar?\n" << "\n" << "barfoo!\n" << "panda\n";
    for (const QByteArray& line : qAsConst(dataToSend)) {
        m_conn->write(line);
    }
    m_conn->flush();

    int maxAttemps = 5;
    while(!m_server->hasPendingConnections() && maxAttemps > 0) {
        --maxAttemps;
        QTest::qSleep(1000);
    }

    QSslSocket* sock = m_server->nextPendingConnection();

    QVERIFY2(sock != nullptr, "Could not open a connection to the client");

    m_reader = new SocketLineReader(sock, this);
    connect(m_reader, &SocketLineReader::readyRead, this, &TestSocketLineReader::newPacket);
    m_timer.start();
    m_loop.exec();

    /* remove the empty line before compare */
    dataToSend.removeOne("\n");

    QCOMPARE(m_packets.count(), 5);//We expect 5 Packets
    for(int x = 0;x < 5; ++x) {
        QCOMPARE(m_packets[x], dataToSend[x]);
    }
}

void TestSocketLineReader::newPacket()
{
    int maxLoops = 5;
    while(m_reader->hasPacketsAvailable() && maxLoops > 0) {
        --maxLoops;
        const QByteArray packet = m_reader->readLine();
        if (!packet.isEmpty()) {
            m_packets.append(packet);
        }

        if (m_packets.count() == 5) {
            m_loop.exit();
        }
    }
}


QTEST_GUILESS_MAIN(TestSocketLineReader)

#include "testsocketlinereader.moc"
