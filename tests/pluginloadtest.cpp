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
#include "kdeconnect-version.h"

class TestDaemon : public Daemon
{
Q_OBJECT
public:
    TestDaemon(QObject* parent = Q_NULLPTR)
        : Daemon(parent, true)
        , m_nam(Q_NULLPTR)
    {
    }

    void requestPairing(Device* d) Q_DECL_OVERRIDE
    {
        d->acceptPairing();
    }

    void reportError(const QString & title, const QString & description) Q_DECL_OVERRIDE
    {
        qWarning() << "error:" << title << description;
    }

    QNetworkAccessManager* networkAccessManager() Q_DECL_OVERRIDE
    {
        if (!m_nam) {
            m_nam = new KIO::AccessManager(this);
        }
        return m_nam;
    }

private:
    QNetworkAccessManager* m_nam;
};

class PluginLoadTest : public QObject
{
    Q_OBJECT
    public:
        PluginLoadTest() : mDaemon(new TestDaemon) {
            QStandardPaths::setTestModeEnabled(true);
        }

    private Q_SLOTS:
        void testPlugins() {
            Device* d = nullptr;
            foreach(Device* id, mDaemon->devicesList()) {
                if (id->isReachable()) {
                    id->requestPair();
                    d = id;
                }
            }

            if (!d->loadedPlugins().contains("kdeconnect_remotecontrol")) {
                QSKIP("kdeconnect_remotecontrol is required for this test");
            }

            QVERIFY(d);
            QVERIFY(d->isPaired());
            QVERIFY(d->isReachable());

            d->setPluginEnabled("kdeconnect_mousepad", false);
            QCOMPARE(d->isPluginEnabled("kdeconnect_mousepad"), false);
            QVERIFY(d->unsupportedPlugins().contains("kdeconnect_remotecontrol"));

            d->setPluginEnabled("kdeconnect_mousepad", true);
            QCOMPARE(d->isPluginEnabled("kdeconnect_mousepad"), true);
            QVERIFY(!d->unsupportedPlugins().contains("kdeconnect_remotecontrol"));
        }

    private:
        TestDaemon* mDaemon;
};

QTEST_MAIN(PluginLoadTest);

#include "pluginloadtest.moc"
