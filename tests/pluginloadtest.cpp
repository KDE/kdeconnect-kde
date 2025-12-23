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

#include "core/daemon.h"
#include "core/device.h"
#include "core/kdeconnectplugin.h"
#include "kdeconnect-version.h"
#include "testdaemon.h"
#include <backends/pairinghandler.h>

class PluginLoadTest : public QObject
{
    Q_OBJECT
public:
    PluginLoadTest()
    {
        QStandardPaths::setTestModeEnabled(true);
        m_daemon = new TestDaemon;
    }

private Q_SLOTS:
    void testPlugins()
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

        if (!device->loadedPlugins().contains(QStringLiteral("kdeconnect_remotecontrol"))) {
            QSKIP("kdeconnect_remotecontrol is required for this test");
        }

        QVERIFY(device);
        QVERIFY(device->isPaired());
        QVERIFY(device->isReachable());

        device->setPluginEnabled(QStringLiteral("kdeconnect_mousepad"), false);
        QCOMPARE(device->isPluginEnabled(QStringLiteral("kdeconnect_mousepad")), false);
        QVERIFY(device->supportedPlugins().contains(QStringLiteral("kdeconnect_remotecontrol")));

        device->setPluginEnabled(QStringLiteral("kdeconnect_mousepad"), true);
        QCOMPARE(device->isPluginEnabled(QStringLiteral("kdeconnect_mousepad")), true);
        QVERIFY(device->supportedPlugins().contains(QStringLiteral("kdeconnect_remotecontrol")));
    }

private:
    TestDaemon *m_daemon;
};

QTEST_MAIN(PluginLoadTest);

#include "pluginloadtest.moc"
