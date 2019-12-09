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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <QSocketNotifier>
#include <kdeconnectconfig.h>
#include <backends/lan/uploadjob.h>
#include <core/filetransferjob.h>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QTest>
#include <QTemporaryFile>
#include <QSignalSpy>
#include <QStandardPaths>

#include "core/daemon.h"
#include "core/device.h"
#include "core/kdeconnectplugin.h"
#include <backends/pairinghandler.h>
#include "kdeconnect-version.h"
#include "testdaemon.h"
#include <plugins/share/shareplugin.h>
#include <backends/lan/compositeuploadjob.h>

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
            if (!(m_daemon->getLinkProviders().size() > 0)) {
                QFAIL("No links available, but loopback should have been provided by the test");
            }

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
            if (d == nullptr) {
                QFAIL("Unable to determine device");
            }
            m_daemon->releaseDiscoveryMode(QStringLiteral("test"));
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
            const QString destFile = QDir::tempPath() + QStringLiteral("/kdeconnect-test-sentfile");
            QFile(destFile).remove();

            const QString deviceId = KdeConnectConfig::instance().deviceId()
                        , deviceName = QStringLiteral("testdevice")
                        , deviceType = KdeConnectConfig::instance().deviceType();

            KdeConnectConfig::instance().addTrustedDevice(deviceId, deviceName, deviceType);
            KdeConnectConfig::instance().setDeviceProperty(deviceId, QStringLiteral("certificate"), QString::fromLatin1(KdeConnectConfig::instance().certificate().toPem())); // Using same certificate from kcc, instead of generating

            //We need the device to be loaded on the daemon, otherwise CompositeUploadJob will get a null device
            Device* device = new Device(this, deviceId);
            m_daemon->addDevice(device);

            QSharedPointer<QFile> f(new QFile(aFile));
            NetworkPacket np(PACKET_TYPE_SHARE_REQUEST);
            np.setPayload(f, f->size());

            CompositeUploadJob* job = new CompositeUploadJob(deviceId, false);
            UploadJob* uj = new UploadJob(np);
            job->addSubjob(uj);

            QSignalSpy spyUpload(job, &KJob::result);
            job->start();

            f->open(QIODevice::ReadWrite);

            FileTransferJob* ft = np.createPayloadTransferJob(QUrl::fromLocalFile(destFile));

            QSignalSpy spyTransfer(ft, &KJob::result);

            ft->start();

            QVERIFY(spyTransfer.count() || spyTransfer.wait());

            if (ft->error()) {
                qWarning() << "fterror" << ft->errorString();
            }

            QCOMPARE(ft->error(), 0);

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
