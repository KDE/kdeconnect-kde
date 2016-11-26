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
    QTimer mTimer;
    QEventLoop mLoop;
    QList<QByteArray> mPackages;
    Server *mServer;
    QSslSocket *mConn;
    SocketLineReader *mReader;
};

void TestSocketLineReader::initTestCase()
{
    mServer = new Server(this);

    QVERIFY2(mServer->listen(QHostAddress::LocalHost, 8694), "Failed to create local tcp server");

    mTimer.setInterval(4000);//For second is more enough to send some data via local socket
    mTimer.setSingleShot(true);
    connect(&mTimer, &QTimer::timeout, &mLoop, &QEventLoop::quit);

    mConn = new QSslSocket(this);
    mConn->connectToHost(QHostAddress::LocalHost, 8694);
    connect(mConn, &QAbstractSocket::connected, &mLoop, &QEventLoop::quit);
    mTimer.start();
    mLoop.exec();

    QVERIFY2(mConn->isOpen(), "Could not connect to local tcp server");
}

void TestSocketLineReader::socketLineReader()
{
    QList<QByteArray> dataToSend;
    dataToSend << "foobar\n" << "barfoo\n" << "foobar?\n" << "\n" << "barfoo!\n" << "panda\n";
    Q_FOREACH(const QByteArray &line, dataToSend) {
        mConn->write(line);
    }
    mConn->flush();

    int maxAttemps = 5;
    while(!mServer->hasPendingConnections() && maxAttemps > 0) {
        --maxAttemps;
        QTest::qSleep(1000);
    }

    QSslSocket *sock = mServer->nextPendingConnection();

    QVERIFY2(sock != nullptr, "Could not open a connection to the client");

    mReader = new SocketLineReader(sock, this);
    connect(mReader, &SocketLineReader::readyRead, this, &TestSocketLineReader::newPackage);
    mTimer.start();
    mLoop.exec();

    /* remove the empty line before compare */
    dataToSend.removeOne("\n");

    QCOMPARE(mPackages.count(), 5);//We expect 5 Packages
    for(int x = 0;x < 5; ++x) {
        QCOMPARE(mPackages[x], dataToSend[x]);
    }
}

void TestSocketLineReader::newPackage()
{
    if (!mReader->bytesAvailable()) {
        return;
    }

    int maxLoops = 5;
    while(mReader->bytesAvailable() > 0 && maxLoops > 0) {
        --maxLoops;
        const QByteArray package = mReader->readLine();
        if (!package.isEmpty()) {
            mPackages.append(package);
        }

        if (mPackages.count() == 5) {
            mLoop.exit();
        }
    }
}


QTEST_GUILESS_MAIN(TestSocketLineReader)

#include "testsocketlinereader.moc"
