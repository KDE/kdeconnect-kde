/**
 * Copyright 2015 Holger Kaelberer <holger.k@elberer.de>
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
#include <QStandardPaths>

#include <kiconloader.h>

#include "core/daemon.h"
#include "core/device.h"
#include "core/kdeconnectplugin.h"
#include "kdeconnect-version.h"
#include "plugins/sendnotifications/sendnotificationsplugin.h"
#include "plugins/sendnotifications/notificationslistener.h"
#include "plugins/sendnotifications/notifyingapplication.h"

// Tweaked NotificationsPlugin for testing
class TestNotificationsPlugin : public SendNotificationsPlugin
{
    Q_OBJECT
public:
    explicit TestNotificationsPlugin(QObject *parent, const QVariantList &args)
    : SendNotificationsPlugin(parent, args)
    {
    }

    ~TestNotificationsPlugin() override = default;

    // allow to access notificationsListener for testing:
    NotificationsListener* getNotificationsListener() const
    {
        return notificationsListener;
    }

    void setNotificationsListener(NotificationsListener* value)
    {
        notificationsListener = value;
    }

};

// Tweaked Device for testing:
class TestDevice: public Device
{
    Q_OBJECT
private:
    int sentPackages;
    NetworkPackage *lastPackage;

public:
    explicit TestDevice(QObject* parent, const QString& id)
        : Device (parent, id)
        , sentPackages(0)
        , lastPackage(nullptr)
    {}

    ~TestDevice() override
    {
        delete lastPackage;
    }

    int getSentPackages() const
    {
        return sentPackages;
    }

    const NetworkPackage* getLastPackage() const
    {
        return lastPackage;
    }

public Q_SLOTS:
    bool sendPackage(NetworkPackage& np) override
    {
        ++sentPackages;
        // copy package manually to allow for inspection (can't use copy-constructor)
        delete lastPackage;
        lastPackage = new NetworkPackage(np.type());
        Q_ASSERT(lastPackage);
        for (QVariantMap::ConstIterator iter = np.body().constBegin();
             iter != np.body().constEnd(); iter++)
            lastPackage->set(iter.key(), iter.value());
        lastPackage->setPayload(np.payload(), np.payloadSize());
        return true;
    }
};

// Tweaked NotificationsListener for testing:
class TestedNotificationsListener: public NotificationsListener
{

public:
    explicit TestedNotificationsListener(KdeConnectPlugin* aPlugin)
        : NotificationsListener(aPlugin)
    {}

    ~TestedNotificationsListener() override
    {}

    QHash<QString, NotifyingApplication>& getApplications()
    {
        return applications;
    }

    void setApplications(const QHash<QString, NotifyingApplication>& value)
    {
        applications = value;
    }

};

class TestNotificationListener : public QObject
{
    Q_OBJECT
    public:
        TestNotificationListener()
            : plugin(nullptr)
        {
            QStandardPaths::setTestModeEnabled(true);
        }

    private Q_SLOTS:
        void testNotify();

    private:
        TestNotificationsPlugin* plugin;
};

void TestNotificationListener::testNotify()
{
    //
    // set things up:
    //

    QString dId("testid");
    TestDevice *d = new TestDevice(nullptr, dId);

    int proxiedNotifications = 0;
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    plugin = new TestNotificationsPlugin(this,
                                         QVariantList({ QVariant::fromValue<Device*>(d),
                                                        "notifications_plugin",
                                                        {"kdeconnect.notification"}}));
    QVERIFY(plugin->getNotificationsListener());
    delete plugin->getNotificationsListener();

    // inject our tweaked NotificationsListener:
    TestedNotificationsListener* listener = new TestedNotificationsListener(plugin);
    QVERIFY(listener);
    plugin->setNotificationsListener(listener);
    QCOMPARE(listener, plugin->getNotificationsListener());

    // make sure config is default:
    plugin->config()->set("generalPersistent", false);
    plugin->config()->set("generalIncludeBody", true);
    plugin->config()->set("generalUrgency", 0);
    QCOMPARE(plugin->config()->get<bool>("generalPersistent"), false);
    QCOMPARE(plugin->config()->get<bool>("generalIncludeBody"), true);
    QCOMPARE(plugin->config()->get<bool>("generalUrgency"), false);

    // applications are modified directly:
    listener->getApplications().clear();
    QCOMPARE(listener->getApplications().count(), 0);

    //
    // Go !!!
    //

    uint replacesId = 99;
    uint retId;
    QString appName("some-appName");
    QString body("some-body");
    QString icon("some-icon");
    QString summary("some-summary");

    // regular Notify call that is synchronized ...
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, {{}}, 0);
    // ... should return replacesId,
    QCOMPARE(retId, replacesId);
    // ... have triggered sending a package
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    // ... with our properties,
    QCOMPARE(d->getLastPackage()->get<uint>("id"), replacesId);
    QCOMPARE(d->getLastPackage()->get<QString>("appName"), appName);
    QCOMPARE(d->getLastPackage()->get<QString>("ticker"), summary + ": " + body);
    QCOMPARE(d->getLastPackage()->get<bool>("isClearable"), true);
    QCOMPARE(d->getLastPackage()->hasPayload(), false);

    // ... and create a new application internally that is initialized correctly:
    QCOMPARE(listener->getApplications().count(), 1);
    QVERIFY(listener->getApplications().contains(appName));
    QVERIFY(listener->getApplications()[appName].active);
    QCOMPARE(listener->getApplications()[appName].name, appName);
    QVERIFY(listener->getApplications()[appName].blacklistExpression.pattern().isEmpty());
    QCOMPARE(listener->getApplications()[appName].name, appName);
    QCOMPARE(listener->getApplications()[appName].icon, icon);

    // another one, with other timeout and urgency values:
    QString appName2("some-appName2");
    QString body2("some-body2");
    QString icon2("some-icon2");
    QString summary2("some-summary2");

    retId = listener->Notify(appName2, replacesId+1, icon2, summary2, body2, {}, {{"urgency", 2}}, 10);
    QCOMPARE(retId, replacesId+1);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    QCOMPARE(d->getLastPackage()->get<uint>("id"), replacesId+1);
    QCOMPARE(d->getLastPackage()->get<QString>("appName"), appName2);
    QCOMPARE(d->getLastPackage()->get<QString>("ticker"), summary2 + ": " + body2);
    QCOMPARE(d->getLastPackage()->get<bool>("isClearable"), false);  // timeout != 0
    QCOMPARE(d->getLastPackage()->hasPayload(), false);
    QCOMPARE(listener->getApplications().count(), 2);
    QVERIFY(listener->getApplications().contains(appName2));
    QVERIFY(listener->getApplications().contains(appName));

    // if persistent-only is set, timeouts > 0 are not synced:
    plugin->config()->set("generalPersistent", true);
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, {{}}, 1);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    retId = listener->Notify(appName2, replacesId, icon2, summary2, body2, {}, {{}}, 3);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    // but timeout == 0 is
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    plugin->config()->set("generalPersistent", false);

    // if min-urgency is set, lower urgency levels are not synced:
    plugin->config()->set("generalUrgency", 1);
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, {{"urgency", 0}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    // equal urgency is
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, {{"urgency", 1}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    // higher urgency as well
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, {{"urgency", 2}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    plugin->config()->set("generalUrgency", 0);

    // notifications for a deactivated application are not synced:
    QVERIFY(listener->getApplications().contains(appName));
    listener->getApplications()[appName].active = false;
    QVERIFY(!listener->getApplications()[appName].active);
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, {{"urgency", 0}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    // others are still:
    retId = listener->Notify(appName2, replacesId+1, icon2, summary2, body2, {}, {{}}, 0);
    QCOMPARE(retId, replacesId+1);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    // back to normal:
    listener->getApplications()[appName].active = true;
    QVERIFY(listener->getApplications()[appName].active);
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());

    // notifications with blacklisted subjects are not synced:
    QVERIFY(listener->getApplications().contains(appName));
    listener->getApplications()[appName].blacklistExpression.setPattern("black[12]|foo(bar|baz)");
    retId = listener->Notify(appName, replacesId, icon, "summary black1", body, {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    retId = listener->Notify(appName, replacesId, icon, "summary foobar", body, {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    // other subjects are synced:
    retId = listener->Notify(appName, replacesId, icon, "summary foo", body, {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    retId = listener->Notify(appName, replacesId, icon, "summary black3", body, {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    // also body is checked by blacklist if requested:
    plugin->config()->set("generalIncludeBody", true);
    retId = listener->Notify(appName, replacesId, icon, summary, "body black1", {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    retId = listener->Notify(appName, replacesId, icon, summary, "body foobaz", {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    // body does not matter if inclusion was not requested:
    plugin->config()->set("generalIncludeBody", false);
    retId = listener->Notify(appName, replacesId, icon, summary, "body black1", {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    // without body, also ticker value is different:
    QCOMPARE(d->getLastPackage()->get<QString>("ticker"), summary);
    retId = listener->Notify(appName, replacesId, icon, summary, "body foobaz", {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());

    // back to normal:
    listener->getApplications()[appName].blacklistExpression.setPattern("");
    plugin->config()->set("generalIncludeBody", true);
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    retId = listener->Notify(appName2, replacesId, icon2, summary2, body2, {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());

    // icon synchronization:
    int count = 0;
    foreach (const auto& iconName, KIconLoader::global()->queryIcons(-KIconLoader::SizeEnormous, KIconLoader::Application)) {
        if (!iconName.endsWith(".png"))
            continue;
        if (count++ > 3) // max 3 iterations
            break;

        // existing icons are sync-ed if requested
        plugin->config()->set("generalSynchronizeIcons", true);
        QFileInfo fi(iconName);
        //qDebug() << "XXX" << iconName << fi.baseName() << fi.size();
        retId = listener->Notify(appName, replacesId, fi.baseName(), summary, body, {}, {{}}, 0);
        QCOMPARE(retId, replacesId);
        QCOMPARE(++proxiedNotifications, d->getSentPackages());
        QVERIFY(d->getLastPackage()->hasPayload());
        QCOMPARE(d->getLastPackage()->payloadSize(), fi.size());

        // otherwise no payload:
        plugin->config()->set("generalSynchronizeIcons", false);
        retId = listener->Notify(appName, replacesId, fi.baseName(), summary, body, {}, {{}}, 0);
        QCOMPARE(retId, replacesId);
        QCOMPARE(++proxiedNotifications, d->getSentPackages());
        QVERIFY(!d->getLastPackage()->hasPayload());
        QCOMPARE(d->getLastPackage()->payloadSize(), 0);
    }
}


QTEST_GUILESS_MAIN(TestNotificationListener);

#include "testnotificationlistener.moc"
