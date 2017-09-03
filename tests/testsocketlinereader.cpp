/*************************************************************************************
 *  Copyright (C) 2014 by Alejandro Fiestas Olivares <afiestas@kde.org>              *
 *                                                                                   *
 *  This library is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU Lesser General Public                       *
 *  License as published by the Free Software Foundation; either                     *
 *  version 2.1 of the License, or (at your option) any later version.               *
 *                                                                                   *
 *  This library is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU                *
 *  Lesser General Public License for more details.                                  *
 *                                                                                   *
 *  You should have received a copy of the GNU Lesser General Public                 *
 *  License along with this library; if not, write to the Free Software              *
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA       *
 *************************************************************************************/

#include "../core/backends/lan/socketlinereader.h"
#include "../core/backends/lan/server.h"

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
    void newPackage();

private Q_SLOTS:
    void socketLineReader();

private:
    QTimer m_timer;
    QEventLoop m_loop;
    QList<QByteArray> m_packages;
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
    for (const QByteArray& line : dataToSend) {
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
    connect(m_reader, &SocketLineReader::readyRead, this, &TestSocketLineReader::newPackage);
    m_timer.start();
    m_loop.exec();

    /* remove the empty line before compare */
    dataToSend.removeOne("\n");

    QCOMPARE(m_packages.count(), 5);//We expect 5 Packages
    for(int x = 0;x < 5; ++x) {
        QCOMPARE(m_packages[x], dataToSend[x]);
    }
}

void TestSocketLineReader::newPackage()
{
    if (!m_reader->bytesAvailable()) {
        return;
    }

    int maxLoops = 5;
    while(m_reader->bytesAvailable() > 0 && maxLoops > 0) {
        --maxLoops;
        const QByteArray package = m_reader->readLine();
        if (!package.isEmpty()) {
            m_packages.append(package);
        }

        if (m_packages.count() == 5) {
            m_loop.exit();
        }
    }
}


QTEST_GUILESS_MAIN(TestSocketLineReader)

#include "testsocketlinereader.moc"
