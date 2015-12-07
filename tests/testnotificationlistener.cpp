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
#include <QSignalSpy>
#include <QStandardPaths>

#include <KIO/AccessManager>

#include "core/daemon.h"
#include "core/device.h"
#include "core/kdeconnectplugin.h"
#include "kdeconnect-version.h"
#include "plugins/notifications/notificationsplugin.h"
#include "plugins/notifications/notificationslistener.h"
#include "plugins/notifications/notificationsdbusinterface.h"
#include "plugins/notifications/notifyingapplication.h"

// Tweaked NotificationsPlugin for testing
class TestNotificationsPlugin : public NotificationsPlugin
{
    Q_OBJECT
public:
    explicit TestNotificationsPlugin(QObject *parent, const QVariantList &args)
        : NotificationsPlugin(parent, args)
    {
        // make notificationPosted() inspectable:
        connect(notificationsDbusInterface, &NotificationsDbusInterface::notificationPosted,
                this, &TestNotificationsPlugin::notificationPosted);
    }

    virtual ~TestNotificationsPlugin() {};

    // allow to access notificationsListener for testing:
    NotificationsListener* getNotificationsListener() const
    {
        return notificationsListener;
    }

    void setNotificationsListener(NotificationsListener* value)
    {
        notificationsListener = value;
    }

    NotificationsDbusInterface* getNotificationsDbusInterface() const
    {
        return notificationsDbusInterface;
    }

Q_SIGNALS:
    void notificationPosted(const QString& publicId);
};

// Tweaked NotificationsListener for testing:
class TestedNotificationsListener: public NotificationsListener
{

public:
    explicit TestedNotificationsListener(KdeConnectPlugin* aPlugin,
                                         NotificationsDbusInterface* aDbusInterface)
        : NotificationsListener(aPlugin, aDbusInterface)
    {}

    virtual ~TestedNotificationsListener()
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
    Device *d = new Device(nullptr, dId); // not setting any parent or we will double free the dbusInterface
    plugin = new TestNotificationsPlugin(this,
                                         QVariantList({ QVariant::fromValue<Device*>(d),
                                                        "notifications_plugin",
                                                        {"kdeconnect.notification"}}));
    QVERIFY(plugin->getNotificationsListener());
    delete plugin->getNotificationsListener();

    // inject our tweaked NotificationsListener:
    TestedNotificationsListener* listener = new TestedNotificationsListener(plugin, plugin->getNotificationsDbusInterface());
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
    int notificationId = 0;
    QSignalSpy spy(plugin, &TestNotificationsPlugin::notificationPosted);

    // regular Notify call that is synchronized ...
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body", {}, {{}}, 0);
    // ... should return replacesId,
    QCOMPARE(retId, replacesId);
    // ... trigger a notificationPosted signal with incremented id
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
    // ... and create a new application internally that is initialized correctly:
    QCOMPARE(listener->getApplications().count(), 1);
    QVERIFY(listener->getApplications().contains("testApp"));
    QVERIFY(listener->getApplications()["testApp"].active);
    QCOMPARE(listener->getApplications()["testApp"].name, QStringLiteral("testApp"));
    QVERIFY(listener->getApplications()["testApp"].blacklistExpression.pattern().isEmpty());
    QCOMPARE(listener->getApplications()["testApp"].name, QStringLiteral("testApp"));
    QCOMPARE(listener->getApplications()["testApp"].icon, QStringLiteral("some-icon"));

    // another one, with other timeout and urgency values:
    retId = listener->Notify("testApp2", replacesId+1, "some-icon2", "summary2", "body2", {}, {{"urgency", 2}}, 10);
    QCOMPARE(retId, replacesId+1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
    QCOMPARE(listener->getApplications().count(), 2);
    QVERIFY(listener->getApplications().contains("testApp2"));
    QVERIFY(listener->getApplications().contains("testApp"));

    // if persistent-only is set, timeouts > 0 are not synced:
    plugin->config()->set("generalPersistent", true);
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body", {}, {{}}, 1);
    QCOMPARE(retId, 0U);
    QCOMPARE(spy.count(), 0);
    retId = listener->Notify("testApp2", replacesId, "some-icon2", "summary2", "body2", {}, {{}}, 3);
    QCOMPARE(retId, 0U);
    QCOMPARE(spy.count(), 0);
    // but timeout == 0 is
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body", {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
    plugin->config()->set("generalPersistent", false);

    // if min-urgency is set, lower urgency levels are not synced:
    plugin->config()->set("generalUrgency", 1);
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body", {}, {{"urgency", 0}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(spy.count(), 0);
    // equal urgency is
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body", {}, {{"urgency", 1}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
    // higher urgency as well
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body", {}, {{"urgency", 2}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
    plugin->config()->set("generalUrgency", 0);

    // notifications for a deactivated application are not synced:
    QVERIFY(listener->getApplications().contains("testApp"));
    listener->getApplications()["testApp"].active = false;
    QVERIFY(!listener->getApplications()["testApp"].active);
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body", {}, {{"urgency", 0}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(spy.count(), 0);
    // others are still:
    retId = listener->Notify("testApp2", replacesId+1, "some-icon2", "summary2", "body2", {}, {{}}, 0);
    QCOMPARE(retId, replacesId+1);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
    // back to normal:
    listener->getApplications()["testApp"].active = true;
    QVERIFY(listener->getApplications()["testApp"].active);
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body", {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));

    // notifications with blacklisted subjects are not synced:
    QVERIFY(listener->getApplications().contains("testApp"));
    listener->getApplications()["testApp"].blacklistExpression.setPattern("black[12]|foo(bar|baz)");
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary black1", "body", {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(spy.count(), 0);
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary foobar", "body", {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(spy.count(), 0);
    // other subjects are synced:
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary foo", "body", {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary black3", "body", {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
    // also body is checked by blacklist if requested:
    plugin->config()->set("generalIncludeBody", true);
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body black1", {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(spy.count(), 0);
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body foobaz", {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(spy.count(), 0);
    // body does not matter if inclusion was not requested:
    plugin->config()->set("generalIncludeBody", false);
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body black1", {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body foobaz", {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));

    // back to normal:
    listener->getApplications()["testApp"].blacklistExpression.setPattern("");
    plugin->config()->set("generalIncludeBody", true);
    retId = listener->Notify("testApp", replacesId, "some-icon", "summary", "body", {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
    retId = listener->Notify("testApp2", replacesId, "some-icon2", "summary2", "body2", {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString::number(++notificationId));
}


QTEST_GUILESS_MAIN(TestNotificationListener);

#include "testnotificationlistener.moc"
