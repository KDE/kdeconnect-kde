/*************************************************************************************
 *  Copyright (C) 2014 by Albert Vaca Cintora <albertvaka@gmail.com>                 *
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

#include "../core/backends/lan/downloadjob.h"

#include <QTest>
#include <QTcpServer>
#include <QTcpSocket>
#include <QProcess>
#include <QEventLoop>
#include <QTimer>
#include <QHostAddress>
#include <KJob>
#include <QSignalSpy>
#include <iostream>

class DownloadJobTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void failToConnectShouldDestroyTheJob();
    void closingTheConnectionShouldDestroyTheJob();

private:
    void initServer();
    void initDownloadJob();
    void awaitToBeDestroyedOrTimeOut();
    void stopServer();

    QPointer<DownloadJob> test;
    QPointer<QTcpServer> mServer;
};

void DownloadJobTest::initServer()
{
    delete mServer;
    mServer = new QTcpServer(this);
    QVERIFY2(mServer->listen(QHostAddress::LocalHost, 8694), "Failed to create local tcp server");
}

void DownloadJobTest::failToConnectShouldDestroyTheJob()
{
    // no initServer
    test = new DownloadJob(QHostAddress::LocalHost, {{"port", 8694}});

    QSignalSpy spy(test.data(), &KJob::finished);
    test->start();

    QVERIFY(spy.count() || spy.wait());

    QCOMPARE(test->error(), 1);
}

void DownloadJobTest::closingTheConnectionShouldDestroyTheJob()
{
    initServer();
    test = new DownloadJob(QHostAddress::LocalHost, {{"port", 8694}});
    QSignalSpy spy(test.data(), &KJob::finished);
    test->start();
    mServer->close();

    QVERIFY(spy.count() || spy.wait(2000));
}

QTEST_GUILESS_MAIN(DownloadJobTest)

#include "downloadjobtest.moc"
