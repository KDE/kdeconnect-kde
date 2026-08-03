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
#include "testdevice.h"
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

    void testSendEmptyFile()
    {
        // An empty file has no payload, but it should still be sent, counted and go through the
        // same composite job machinery as any other file (fixes empty files being dropped from the
        // total count, or transfers stalling/crashing when an empty file is mixed in with others).
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

        QTemporaryFile temp;
        temp.open();
        temp.close();
        QCOMPARE(QFileInfo(temp.fileName()).size(), 0);

        KdeConnectPlugin *plugin = device->plugin(QStringLiteral("kdeconnect_share"));
        QVERIFY(plugin);
        plugin->metaObject()->invokeMethod(plugin, "shareUrl", Q_ARG(QString, QUrl::fromLocalFile(temp.fileName()).toString()));

        QSignalSpy spy(plugin, SIGNAL(shareReceived(QString)));
        QVERIFY(spy.wait(2000));

        QVariantList args = spy.takeFirst();
        QUrl sentFile(args.first().toUrl());

        QFile file(sentFile.toLocalFile());
        QVERIFY(file.exists());
        QCOMPARE(file.size(), 0);
    }

    void testSslJobs()
    {
        const QString aFile = QFINDTESTDATA("sendfiletest.cpp");
        const QString destFile = QDir::tempPath() + QStringLiteral("/kdeconnect-test-sentfile");
        QFile(destFile).remove();

        DeviceInfo deviceInfo = KdeConnectConfig::instance().deviceInfo();
        KdeConnectConfig::instance().addTrustedDevice(deviceInfo);

        Device *device = new Device(this, deviceInfo.id);
        m_daemon->addDevice(device);

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

    void testUploadEmptyFile()
    {
        // A packet with no payload set at all (like the ones sent for empty files) must still
        // flow through CompositeUploadJob/UploadJob as a normal, immediately-completing subjob,
        // so it gets counted instead of being silently dropped from the total.
        const QString destFile = QDir::tempPath() + QStringLiteral("/kdeconnect-test-empty-upload");
        QFile(destFile).remove();

        DeviceInfo deviceInfo = KdeConnectConfig::instance().deviceInfo();
        KdeConnectConfig::instance().addTrustedDevice(deviceInfo);

        Device *device = new Device(this, deviceInfo.id);
        m_daemon->addDevice(device);

        NetworkPacket np(PACKET_TYPE_SHARE_REQUEST);
        np.set<QString>(QStringLiteral("filename"), QStringLiteral("empty.txt"));
        QVERIFY(!np.hasPayload());

        CompositeUploadJob *job = new CompositeUploadJob(device, false);
        UploadJob *uj = new UploadJob(np);
        job->addSubjob(uj);

        // Like testSslJobs(), sending the packet over the (fake) device link is not actually
        // exercised here; we only check that the no-payload subjob completes on its own instead
        // of hanging forever waiting for a socket connection that will never come.
        QSignalSpy spyUpload(job, &KJob::result);
        job->start();

        QVERIFY(spyUpload.count() || spyUpload.wait());
        // Once finished (however it finished), the job must not be mistaken for still-usable by
        // whoever holds onto it (e.g. LanDeviceLink), or a later file would be silently attached
        // to this already-finished job and never actually get sent.
        QVERIFY(!job->isRunning());

        FileTransferJob *ft = np.createPayloadTransferJob(QUrl::fromLocalFile(destFile));
        QSignalSpy spyTransfer(ft, &KJob::result);
        ft->start();

        QVERIFY(spyTransfer.count() || spyTransfer.wait());
        QCOMPARE(ft->error(), 0);

        QFile resultFile(destFile);
        QVERIFY(resultFile.exists());
        QCOMPARE(resultFile.size(), 0);
    }

    void testMultipleEmptyFilesInOneCompositeUpload()
    {
        // Regression test for a bug where sending several empty files in one go got the count
        // wrong and the transfer stuck: for each file, CompositeUploadJob sends a per-file
        // "header" packet to announce the transfer, and that packet must go straight out over
        // the wire rather than be mistaken for yet another new file to enqueue. For files with a
        // payload, the header's payload device was explicitly stripped before sending; for files
        // with no payload (like empty files, which still carry a valid-but-empty QIODevice per
        // SharePlugin::shareUrl()) it wasn't - so LanDeviceLink treated it as a new file, feeding
        // it back into the same composite job and leaving it stuck waiting for a subjob that
        // would never actually arrive (see bug report: 3 empty files shown as "1 of 4", stuck).
        DeviceInfo deviceInfo = KdeConnectConfig::instance().deviceInfo();
        KdeConnectConfig::instance().addTrustedDevice(deviceInfo);

        TestDevice *device = new TestDevice(this, deviceInfo.id);

        QTemporaryFile empty;
        empty.open();
        empty.close();

        CompositeUploadJob *job = new CompositeUploadJob(device, false);
        for (int i = 0; i < 3; ++i) {
            QSharedPointer<QFile> f(new QFile(empty.fileName()));
            NetworkPacket np(PACKET_TYPE_SHARE_REQUEST);
            np.setPayload(f, 0);
            np.set<QString>(QStringLiteral("filename"), QStringLiteral("empty%1.txt").arg(i));
            QVERIFY(!np.hasPayload());
            job->addSubjob(new UploadJob(np));
        }

        QSignalSpy spyUpload(job, &KJob::result);
        job->start();

        QVERIFY(spyUpload.count() || spyUpload.wait());
        QCOMPARE(job->error(), 0);
        QVERIFY(!job->isRunning());

        // Exactly one header packet per file must have gone out - not fewer (stuck) and not more
        // (a file's own header being mistaken for another new file).
        QCOMPARE(device->getSentPackets(), 3);

        NetworkPacket *last = device->getLastPacket();
        QVERIFY(last);
        QCOMPARE(last->type(), QString(PACKET_TYPE_SHARE_REQUEST));
        QCOMPARE(last->get<int>(QStringLiteral("numberOfFiles")), 3);
        // The header must not carry the payload device onward: it's just an announcement, not
        // something the other end should try to open a transfer socket for.
        QVERIFY(!last->payload());
    }

    void testMoreDataThanAnnounced()
    {
        // A sender can announce a size that turns out to be smaller than what it then sends,
        // for instance when the file it reads is being rewritten as it goes. The file arrives
        // whole, so it has to be kept.
        const QString aFile = QFINDTESTDATA("sendfiletest.cpp");
        const QString destFile = QDir::tempPath() + QStringLiteral("/kdeconnect-test-longer-than-announced");
        QFile(destFile).remove();

        DeviceInfo deviceInfo = KdeConnectConfig::instance().deviceInfo();
        KdeConnectConfig::instance().addTrustedDevice(deviceInfo);

        Device *device = new Device(this, deviceInfo.id);
        m_daemon->addDevice(device);

        QSharedPointer<QFile> f(new QFile(aFile));
        const qint64 actualSize = f->size();
        QVERIFY(actualSize > 16);

        NetworkPacket np(PACKET_TYPE_SHARE_REQUEST);
        np.setPayload(f, actualSize - 16);

        CompositeUploadJob *job = new CompositeUploadJob(device, false);
        UploadJob *uj = new UploadJob(np);
        job->addSubjob(uj);
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

        QFile resultFile(destFile);
        QVERIFY(resultFile.exists());
        QCOMPARE(resultFile.size(), actualSize);
    }

    void testUnannouncedSize()
    {
        // A sender that does not know how big the payload is announces a size of -1, which the
        // Android app does whenever the content provider has no size to give. Nothing was
        // promised, so nothing can fall short of it.
        const QString aFile = QFINDTESTDATA("sendfiletest.cpp");
        const QString destFile = QDir::tempPath() + QStringLiteral("/kdeconnect-test-unannounced-size");
        QFile(destFile).remove();

        DeviceInfo deviceInfo = KdeConnectConfig::instance().deviceInfo();
        KdeConnectConfig::instance().addTrustedDevice(deviceInfo);

        Device *device = new Device(this, deviceInfo.id);
        m_daemon->addDevice(device);

        QSharedPointer<QFile> f(new QFile(aFile));
        const qint64 actualSize = f->size();

        NetworkPacket np(PACKET_TYPE_SHARE_REQUEST);
        np.setPayload(f, -1);

        CompositeUploadJob *job = new CompositeUploadJob(device, false);
        UploadJob *uj = new UploadJob(np);
        job->addSubjob(uj);
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

        QFile resultFile(destFile);
        QVERIFY(resultFile.exists());
        QCOMPARE(resultFile.size(), actualSize);
    }

    void testTimeout()
    {
        const QString aFile = QFINDTESTDATA("sendfiletest.cpp");

        DeviceInfo deviceInfo = KdeConnectConfig::instance().deviceInfo();
        KdeConnectConfig::instance().addTrustedDevice(deviceInfo);

        TestDevice *device = new TestDevice(this, deviceInfo.id);
        m_daemon->addDevice(device);

        QSharedPointer<QFile> f(new QFile(aFile));
        NetworkPacket np(PACKET_TYPE_SHARE_REQUEST);
        np.setPayload(f, f->size());

        CompositeUploadJob *job = new CompositeUploadJob(device, false);
#ifdef BUILD_TESTING
        job->testSetTimeoutMs(1000);
#endif
        UploadJob *uj = new UploadJob(np);
        job->addSubjob(uj);

        QSignalSpy spy(job, &KJob::result);
        job->start();

        QVERIFY(spy.wait(5000));
        QCOMPARE(job->error(), CompositeUploadJob::ConnectionTimeoutError);
    }

private:
    TestDaemon *m_daemon;
};

QTEST_MAIN(TestSendFile);

#include "sendfiletest.moc"
