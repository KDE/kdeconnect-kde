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

class PluginLoadTest : public QObject
{
    Q_OBJECT
    public:
        PluginLoadTest() {
            QStandardPaths::setTestModeEnabled(true);
            m_daemon = new TestDaemon;
        }

    private Q_SLOTS:
        void testPlugins() {
            Device* d = nullptr;
            m_daemon->acquireDiscoveryMode(QStringLiteral("plugintest"));
            const QList<Device*> devicesList = m_daemon->devicesList();
            for (Device* id : devicesList) {
                if (id->isReachable()) {
                    if (!id->isTrusted())
                        id->requestPair();
                    d = id;
                    break;
                }
            }
            m_daemon->releaseDiscoveryMode(QStringLiteral("plugintest"));

            if (!d->loadedPlugins().contains(QStringLiteral("kdeconnect_remotecontrol"))) {
                QSKIP("kdeconnect_remotecontrol is required for this test");
            }

            QVERIFY(d);
            QVERIFY(d->isTrusted());
            QVERIFY(d->isReachable());

            d->setPluginEnabled(QStringLiteral("kdeconnect_mousepad"), false);
            QCOMPARE(d->isPluginEnabled("kdeconnect_mousepad"), false);
            QVERIFY(d->supportedPlugins().contains("kdeconnect_remotecontrol"));

            d->setPluginEnabled(QStringLiteral("kdeconnect_mousepad"), true);
            QCOMPARE(d->isPluginEnabled("kdeconnect_mousepad"), true);
            QVERIFY(d->supportedPlugins().contains("kdeconnect_remotecontrol"));
        }

    private:
        TestDaemon* m_daemon;
};

QTEST_MAIN(PluginLoadTest);

#include "pluginloadtest.moc"
