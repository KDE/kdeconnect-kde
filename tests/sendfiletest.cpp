/**
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QSocketNotifier>
#include <backends/lan/downloadjob.h>
#include <kdeconnectconfig.h>
#include <backends/lan/uploadjob.h>
#include <core/filetransferjob.h>
#include <QApplication>
#include <QNetworkAccessManager>
#include <QTest>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <QStandardPaths>

#include <KIO/AccessManager>

#include "core/daemon.h"
#include "core/device.h"
#include "core/kdeconnectplugin.h"
#include <backends/pairinghandler.h>
#include "kdeconnect-version.h"
#include "testdaemon.h"

class TestSendFile : public QObject
{
    Q_OBJECT
    public:
        TestSendFile() {
            QStandardPaths::setTestModeEnabled(true);
            m_daemon = new TestDaemon;
        }

    private Q_SLOTS:
        void testSend() {
            m_daemon->acquireDiscoveryMode(QStringLiteral("test"));
            Device* d = nullptr;
            const QList<Device*> devicesList = m_daemon->devicesList();
            for (Device* id : devicesList) {
                if (id->isReachable()) {
                    if (!id->isTrusted())
                        id->requestPair();
                    d = id;
                }
            }
            m_daemon->releaseDiscoveryMode(QStringLiteral("test"));
            QVERIFY(d);
            QCOMPARE(d->isReachable(), true);
            QCOMPARE(d->isTrusted(), true);

            QByteArray content("12312312312313213123213123");

            QTemporaryFile temp;
            temp.open();
            temp.write(content);
            temp.close();

            KdeConnectPlugin* plugin = d->plugin(QStringLiteral("kdeconnect_share"));
            QVERIFY(plugin);
            plugin->metaObject()->invokeMethod(plugin, "shareUrl", Q_ARG(QString, QUrl::fromLocalFile(temp.fileName()).toString()));

            QSignalSpy spy(plugin, SIGNAL(shareReceived(QString)));
            QVERIFY(spy.wait(2000));

            QVariantList args = spy.takeFirst();
            QUrl sentFile(args.first().toUrl());

            QFile file(sentFile.toLocalFile());
            QCOMPARE(file.size(), content.size());
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), content);
        }

        void testSslJobs()
        {
            const QString aFile = QFINDTESTDATA("sendfiletest.cpp");
            const QString destFile = QDir::tempPath() + "/kdeconnect-test-sentfile";
            QFile(destFile).remove();

            const QString deviceId = KdeConnectConfig::instance()->deviceId()
                        , deviceName = QStringLiteral("testdevice")
                        , deviceType = KdeConnectConfig::instance()->deviceType();

            KdeConnectConfig* kcc = KdeConnectConfig::instance();
            kcc->addTrustedDevice(deviceId, deviceName, deviceType);
            kcc->setDeviceProperty(deviceId, QStringLiteral("certificate"), QString::fromLatin1(kcc->certificate().toPem())); // Using same certificate from kcc, instead of generating

            QSharedPointer<QFile> f(new QFile(aFile));
            UploadJob* uj = new UploadJob(f, deviceId);
            QSignalSpy spyUpload(uj, &KJob::result);
            uj->start();

            auto info = uj->transferInfo();
            info.insert(QStringLiteral("deviceId"), deviceId);
            info.insert(QStringLiteral("size"), aFile.size());

            DownloadJob* dj = new DownloadJob(QHostAddress::LocalHost, info);

            QVERIFY(dj->getPayload()->open(QIODevice::ReadOnly));

            FileTransferJob* ft = new FileTransferJob(dj->getPayload(), uj->transferInfo()[QStringLiteral("size")].toInt(), QUrl::fromLocalFile(destFile));

            QSignalSpy spyDownload(dj, &KJob::result);
            QSignalSpy spyTransfer(ft, &KJob::result);

            ft->start();
            dj->start();

            QVERIFY(spyTransfer.count() || spyTransfer.wait(1000000000));

            if (ft->error()) qWarning() << "fterror" << ft->errorString();

            QCOMPARE(ft->error(), 0);
            QCOMPARE(spyDownload.count(), 1);
            QCOMPARE(spyUpload.count(), 1);

            QFile resultFile(destFile), originFile(aFile);
            QVERIFY(resultFile.open(QIODevice::ReadOnly));
            QVERIFY(originFile.open(QIODevice::ReadOnly));

            const QByteArray resultContents = resultFile.readAll(), originContents = originFile.readAll();
            QCOMPARE(resultContents.size(), originContents.size());
            QCOMPARE(resultFile.readAll(), originFile.readAll());
        }

    private:
        TestDaemon* m_daemon;
};

QTEST_MAIN(TestSendFile);

#include "sendfiletest.moc"
