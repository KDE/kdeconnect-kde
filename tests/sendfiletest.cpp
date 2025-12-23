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

        const auto deviceIds = m_daemon->devices();
        Device *device = nullptr;
        for (const QString &deviceId : deviceIds) {
            Device *d = m_daemon->getDevice(deviceId);
            if (d->isReachable()) {
                if (!d->isPaired())
                    d->requestPairing();
                device = d;
            }
        }
        if (device == nullptr) {
            QFAIL("Unable to determine device");
        }
        QCOMPARE(device->isReachable(), true);
        QCOMPARE(device->isPaired(), true);

        QByteArray content("12312312312313213123213123");

        QTemporaryFile temp;
        temp.open();
        temp.write(content);
        temp.close();

        KdeConnectPlugin *plugin = device->plugin(QStringLiteral("kdeconnect_share"));
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

        DeviceInfo deviceInfo = KdeConnectConfig::instance().deviceInfo();
        KdeConnectConfig::instance().addTrustedDevice(deviceInfo);

        auto d = std::make_unique<Device>(deviceInfo.id);
        auto device = d.get();
        m_daemon->addDevice(std::move(d));

        QSharedPointer<QFile> f(new QFile(aFile));
        NetworkPacket np(PACKET_TYPE_SHARE_REQUEST);
        np.setPayload(f, f->size());

        CompositeUploadJob *job = new CompositeUploadJob(device, false);
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
