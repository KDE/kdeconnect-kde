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

    QPointer<DownloadJob> m_test;
    QPointer<QTcpServer> m_server;
};

void DownloadJobTest::initServer()
{
    delete m_server;
    m_server = new QTcpServer(this);
    QVERIFY2(m_server->listen(QHostAddress::LocalHost, 8694), "Failed to create local tcp server");
}

void DownloadJobTest::failToConnectShouldDestroyTheJob()
{
    // no initServer
    m_test = new DownloadJob(QHostAddress::LocalHost, {{"port", 8694}});

    QSignalSpy spy(m_test.data(), &KJob::finished);
    m_test->start();

    QVERIFY(spy.count() || spy.wait());

    QCOMPARE(m_test->error(), 1);
}

void DownloadJobTest::closingTheConnectionShouldDestroyTheJob()
{
    initServer();
    m_test = new DownloadJob(QHostAddress::LocalHost, {{"port", 8694}});
    QSignalSpy spy(m_test.data(), &KJob::finished);
    m_test->start();
    m_server->close();

    QVERIFY(spy.count() || spy.wait(2000));
}

QTEST_GUILESS_MAIN(DownloadJobTest)

#include "downloadjobtest.moc"
