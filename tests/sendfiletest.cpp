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
            mDaemon = new TestDaemon;
        }

    private Q_SLOTS:
        void testSend() {
            mDaemon->acquireDiscoveryMode("test");
            Device* d = nullptr;
            foreach(Device* id, mDaemon->devicesList()) {
                if (id->isReachable()) {
                    if (!id->isTrusted())
                        id->requestPair();
                    d = id;
                }
            }
            mDaemon->releaseDiscoveryMode("test");
            QVERIFY(d);
            QCOMPARE(d->isReachable(), true);
            QCOMPARE(d->isTrusted(), true);

            QByteArray content("12312312312313213123213123");

            QTemporaryFile temp;
            temp.open();
            temp.write(content);
            temp.close();

            KdeConnectPlugin* plugin = d->plugin("kdeconnect_share");
            QVERIFY(plugin);
            plugin->metaObject()->invokeMethod(plugin, "shareUrl", Q_ARG(QString, QUrl::fromLocalFile(temp.fileName()).toString()));

            QSignalSpy spy(plugin, SIGNAL(shareReceived(QUrl)));
            QVERIFY(spy.wait(2000));

            QVariantList args = spy.takeFirst();
            QUrl sentFile(args.first().toUrl());

            QFile file(sentFile.toLocalFile());
            QCOMPARE(file.size(), content.size());
            QVERIFY(file.open(QIODevice::ReadOnly));
            QCOMPARE(file.readAll(), content);
        }

    private:
        TestDaemon* mDaemon;
};

QTEST_MAIN(TestSendFile);

#include "sendfiletest.moc"
