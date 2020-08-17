/**
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include <QSocketNotifier>
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
            if (!(m_daemon->getLinkProviders().size() > 0)) {
                QFAIL("No links available, but loopback should have been provided by the test");
            }

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
            if (d == nullptr) {
                QFAIL("Unable to determine device");
            }
            m_daemon->releaseDiscoveryMode(QStringLiteral("plugintest"));

            if (!d->loadedPlugins().contains(QStringLiteral("kdeconnect_remotecontrol"))) {
                QSKIP("kdeconnect_remotecontrol is required for this test");
            }

            QVERIFY(d);
            QVERIFY(d->isTrusted());
            QVERIFY(d->isReachable());

            d->setPluginEnabled(QStringLiteral("kdeconnect_mousepad"), false);
            QCOMPARE(d->isPluginEnabled(QStringLiteral("kdeconnect_mousepad")), false);
            QVERIFY(d->supportedPlugins().contains(QStringLiteral("kdeconnect_remotecontrol")));

            d->setPluginEnabled(QStringLiteral("kdeconnect_mousepad"), true);
            QCOMPARE(d->isPluginEnabled(QStringLiteral("kdeconnect_mousepad")), true);
            QVERIFY(d->supportedPlugins().contains(QStringLiteral("kdeconnect_remotecontrol")));
        }

    private:
        TestDaemon* m_daemon;
};

QTEST_MAIN(PluginLoadTest);

#include "pluginloadtest.moc"
