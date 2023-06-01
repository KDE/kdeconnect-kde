/**
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QCoreApplication>
#include <QSignalSpy>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTest>
#include <backends/lan/uploadjob.h>
#include <core/filetransferjob.h>
#include <kdeconnectconfig.h>

#include "core/daemon.h"
#include "core/device.h"
#include "core/kdeconnectplugin.h"
#include "kdeconnect-version.h"
#include "testdaemon.h"
#include <backends/lan/compositeuploadjob.h>
#include <backends/pairinghandler.h>
#include <plugins/share/shareplugin.h>

class TestSendFile : public QObject
{
    Q_OBJECT
public:
    TestSendFile()
    {
        QStandardPaths::setTestModeEnabled(true);
        m_daemon = new TestDaemon;
    }

private Q_SLOTS:
    void testSend()
    {
        if (!(m_daemon->getLinkProviders().size() > 0)) {
            QFAIL("No links available, but loopback should have been provided by the test");
        }

        Device *d = nullptr;
        const QList<Device *> devicesList = m_daemon->devicesList();
        for (Device *id : devicesList) {
            if (id->isReachable()) {
                if (!id->isPaired())
                    id->requestPairing();
                d = id;
            }
        }
        if (d == nullptr) {
            QFAIL("Unable to determine device");
        }
        QCOMPARE(d->isReachable(), true);
        QCOMPARE(d->isPaired(), true);

        QByteArray content("12312312312313213123213123");

        QTemporaryFile temp;
        temp.open();
        temp.write(content);
        temp.close();

        KdeConnectPlugin *plugin = d->plugin(QStringLiteral("kdeconnect_share"));
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

        const QString deviceId = KdeConnectConfig::instance().deviceId(), deviceName = QStringLiteral("testdevice"),
                      deviceType = KdeConnectConfig::instance().deviceType();

        KdeConnectConfig::instance().addTrustedDevice(deviceId, deviceName, deviceType);
        KdeConnectConfig::instance().setDeviceProperty(
            deviceId,
            QStringLiteral("certificate"),
            QString::fromLatin1(KdeConnectConfig::instance().certificate().toPem())); // Using same certificate from kcc, instead of generating

        // We need the device to be loaded on the daemon, otherwise CompositeUploadJob will get a null device
        Device *device = new Device(this, deviceId);
        m_daemon->addDevice(device);

        QSharedPointer<QFile> f(new QFile(aFile));
        NetworkPacket np(PACKET_TYPE_SHARE_REQUEST);
        np.setPayload(f, f->size());

        CompositeUploadJob *job = new CompositeUploadJob(deviceId, false);
        UploadJob *uj = new UploadJob(np);
        job->addSubjob(uj);

        QSignalSpy spyUpload(job, &KJob::result);
        job->start();

        f->open(QIODevice::ReadWrite);

        FileTransferJob *ft = np.createPayloadTransferJob(QUrl::fromLocalFile(destFile));

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
    TestDaemon *m_daemon;
};

QTEST_MAIN(TestSendFile);

#include "sendfiletest.moc"
