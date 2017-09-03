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
#include <QBuffer>
#include <QStandardPaths>
#include <QImage>
#include <QColor>

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
    explicit TestNotificationsPlugin(QObject* parent, const QVariantList& args)
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
    NetworkPackage* lastPackage;

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

    NetworkPackage* getLastPackage()
    {
        return lastPackage;
    }

    void deleteLastPackage()
    {
        delete lastPackage;
        lastPackage = nullptr;
    }

public Q_SLOTS:
    bool sendPackage(NetworkPackage& np) override
    {
        ++sentPackages;
        // copy package manually to allow for inspection (can't use copy-constructor)
        deleteLastPackage();
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
    Q_OBJECT
public:
    explicit TestedNotificationsListener(KdeConnectPlugin* aPlugin)
        : NotificationsListener(aPlugin)
    {}

    ~TestedNotificationsListener() override
    {}

    QHash<QString, NotifyingApplication>& getApplications()
    {
        return m_applications;
    }

    void setApplications(const QHash<QString, NotifyingApplication>& value)
    {
        m_applications = value;
    }

protected:
    bool parseImageDataArgument(const QVariant& argument, int& width,
                                int& height, int& rowStride, int& bitsPerSample,
                                int& channels, bool& hasAlpha,
                                QByteArray& imageData) const override
    {
        width = argument.toMap().value(QStringLiteral("width")).toInt();
        height = argument.toMap().value(QStringLiteral("height")).toInt();
        rowStride = argument.toMap().value(QStringLiteral("rowStride")).toInt();
        bitsPerSample = argument.toMap().value(QStringLiteral("bitsPerSample")).toInt();
        channels = argument.toMap().value(QStringLiteral("channels")).toInt();
        hasAlpha = argument.toMap().value(QStringLiteral("hasAlpha")).toBool();
        imageData = argument.toMap().value(QStringLiteral("imageData")).toByteArray();
        return true;
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

    QString dId(QStringLiteral("testid"));
    TestDevice* d = new TestDevice(nullptr, dId);

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
    plugin->config()->set(QStringLiteral("generalPersistent"), false);
    plugin->config()->set(QStringLiteral("generalIncludeBody"), true);
    plugin->config()->set(QStringLiteral("generalUrgency"), 0);
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
    QString appName(QStringLiteral("some-appName"));
    QString body(QStringLiteral("some-body"));
    QString icon(QStringLiteral("some-icon"));
    QString summary(QStringLiteral("some-summary"));

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
    QString appName2(QStringLiteral("some-appName2"));
    QString body2(QStringLiteral("some-body2"));
    QString icon2(QStringLiteral("some-icon2"));
    QString summary2(QStringLiteral("some-summary2"));

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
    plugin->config()->set(QStringLiteral("generalPersistent"), true);
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
    plugin->config()->set(QStringLiteral("generalPersistent"), false);

    // if min-urgency is set, lower urgency levels are not synced:
    plugin->config()->set(QStringLiteral("generalUrgency"), 1);
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
    plugin->config()->set(QStringLiteral("generalUrgency"), 0);

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
    listener->getApplications()[appName].blacklistExpression.setPattern(QStringLiteral("black[12]|foo(bar|baz)"));
    retId = listener->Notify(appName, replacesId, icon, QStringLiteral("summary black1"), body, {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    retId = listener->Notify(appName, replacesId, icon, QStringLiteral("summary foobar"), body, {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    // other subjects are synced:
    retId = listener->Notify(appName, replacesId, icon, QStringLiteral("summary foo"), body, {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    retId = listener->Notify(appName, replacesId, icon, QStringLiteral("summary black3"), body, {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    // also body is checked by blacklist if requested:
    plugin->config()->set(QStringLiteral("generalIncludeBody"), true);
    retId = listener->Notify(appName, replacesId, icon, summary, QStringLiteral("body black1"), {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    retId = listener->Notify(appName, replacesId, icon, summary, QStringLiteral("body foobaz"), {}, {{}}, 0);
    QCOMPARE(retId, 0U);
    QCOMPARE(proxiedNotifications, d->getSentPackages());
    // body does not matter if inclusion was not requested:
    plugin->config()->set(QStringLiteral("generalIncludeBody"), false);
    retId = listener->Notify(appName, replacesId, icon, summary, QStringLiteral("body black1"), {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    // without body, also ticker value is different:
    QCOMPARE(d->getLastPackage()->get<QString>("ticker"), summary);
    retId = listener->Notify(appName, replacesId, icon, summary, QStringLiteral("body foobaz"), {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());

    // back to normal:
    listener->getApplications()[appName].blacklistExpression.setPattern(QLatin1String(""));
    plugin->config()->set(QStringLiteral("generalIncludeBody"), true);
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    retId = listener->Notify(appName2, replacesId, icon2, summary2, body2, {}, {{}}, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());

    // icon synchronization:
    QStringList iconPaths;
    // appIcon
    int count = 0;
    const QStringList icons = KIconLoader::global()->queryIcons(-KIconLoader::SizeEnormous, KIconLoader::Application);
    for (const auto& iconName : icons) {
        if (!iconName.endsWith(QLatin1String(".png")))
            continue;
        if (count++ > 3) // max 3 iterations
            break;
        iconPaths.append(iconName); // memorize some paths for later

        // existing icons are sync-ed if requested
        plugin->config()->set(QStringLiteral("generalSynchronizeIcons"), true);
        QFileInfo fi(iconName);
        retId = listener->Notify(appName, replacesId, fi.baseName(), summary, body, {}, {{}}, 0);
        QCOMPARE(retId, replacesId);
        QCOMPARE(++proxiedNotifications, d->getSentPackages());
        QVERIFY(d->getLastPackage()->hasPayload());
        QCOMPARE(d->getLastPackage()->payloadSize(), fi.size());
        // works also with abolute paths
        retId = listener->Notify(appName, replacesId, iconName, summary, body, {}, {{}}, 0);
        QCOMPARE(retId, replacesId);
        QCOMPARE(++proxiedNotifications, d->getSentPackages());
        QVERIFY(d->getLastPackage()->hasPayload());
        QCOMPARE(d->getLastPackage()->payloadSize(), fi.size());
        // extensions other than png are not accepted:
        retId = listener->Notify(appName, replacesId, iconName + ".svg", summary, body, {}, {{}}, 0);
        QCOMPARE(retId, replacesId);
        QCOMPARE(++proxiedNotifications, d->getSentPackages());
        QVERIFY(!d->getLastPackage()->hasPayload());

        // if sync not requested no payload:
        plugin->config()->set(QStringLiteral("generalSynchronizeIcons"), false);
        retId = listener->Notify(appName, replacesId, fi.baseName(), summary, body, {}, {{}}, 0);
        QCOMPARE(retId, replacesId);
        QCOMPARE(++proxiedNotifications, d->getSentPackages());
        QVERIFY(!d->getLastPackage()->hasPayload());
        QCOMPARE(d->getLastPackage()->payloadSize(), 0);
    }
    plugin->config()->set(QStringLiteral("generalSynchronizeIcons"), true);

    // image-path in hints
    if (iconPaths.size() > 0) {
        retId = listener->Notify(appName, replacesId, iconPaths.size() > 1 ? iconPaths[1] : icon, summary, body, {}, {{"image-path", iconPaths[0]}}, 0);
        QCOMPARE(retId, replacesId);
        QCOMPARE(++proxiedNotifications, d->getSentPackages());
        QVERIFY(d->getLastPackage()->hasPayload());
        QFileInfo hintsFi(iconPaths[0]);
        // image-path has priority over appIcon parameter:
        QCOMPARE(d->getLastPackage()->payloadSize(), hintsFi.size());
    }

    // image_path in hints
    if (iconPaths.size() > 0) {
        retId = listener->Notify(appName, replacesId, iconPaths.size() > 1 ? iconPaths[1] : icon, summary, body, {}, {{"image_path", iconPaths[0]}}, 0);
        QCOMPARE(retId, replacesId);
        QCOMPARE(++proxiedNotifications, d->getSentPackages());
        QVERIFY(d->getLastPackage()->hasPayload());
        QFileInfo hintsFi(iconPaths[0]);
        // image_path has priority over appIcon parameter:
        QCOMPARE(d->getLastPackage()->payloadSize(), hintsFi.size());
    }

    // image-data in hints
    // set up:
    QBuffer* buffer;
    QImage image;
    int width = 2, height = 2, rowStride = 4*width, bitsPerSample = 8,
            channels = 4;
    bool hasAlpha = 1;
    char rawData[] = { 0x01, 0x02, 0x03, 0x04,  // raw rgba data
                       0x11, 0x12, 0x13, 0x14,
                       0x21, 0x22, 0x23, 0x24,
                       0x31, 0x32, 0x33, 0x34 };
    QVariantMap imageData = {{"width", width}, {"height", height}, {"rowStride", rowStride},
                            {"bitsPerSample", bitsPerSample}, {"channels", channels},
                            {"hasAlpha", hasAlpha}, {"imageData", QByteArray(rawData, sizeof(rawData))}};
    QVariantMap hints;
#define COMPARE_PIXEL(x, y) \
    QCOMPARE(qRed(image.pixel(x,y)), (int)rawData[x*4 + y*rowStride + 0]); \
    QCOMPARE(qGreen(image.pixel(x,y)), (int)rawData[x*4 + y*rowStride + 1]); \
    QCOMPARE(qBlue(image.pixel(x,y)), (int)rawData[x*4 + y*rowStride + 2]); \
    QCOMPARE(qAlpha(image.pixel(x,y)), (int)rawData[x*4 + y*rowStride + 3]);

    hints.insert(QStringLiteral("image-data"), imageData);
    if (iconPaths.size() > 0)
        hints.insert(QStringLiteral("image-path"), iconPaths[0]);
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, hints, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    QVERIFY(d->getLastPackage()->hasPayload());
    buffer = dynamic_cast<QBuffer*>(d->getLastPackage()->payload().data());
    QCOMPARE(d->getLastPackage()->payloadSize(), buffer->size());
    // image-data is attached as png data
    QVERIFY(image.loadFromData(reinterpret_cast<const uchar*>(buffer->data().constData()), buffer->size(), "PNG"));
    // image-data has priority over image-path:
    QCOMPARE(image.byteCount(), rowStride*height);
    // rgba -> argb conversion was done correctly:
    COMPARE_PIXEL(0,0);
    COMPARE_PIXEL(1,0);
    COMPARE_PIXEL(0,1);
    COMPARE_PIXEL(1,1);

    // same for image_data in hints
    hints.clear();
    hints.insert(QStringLiteral("image-data"), imageData);
    if (iconPaths.size() > 0)
        hints.insert(QStringLiteral("image_path"), iconPaths[0]);
    retId = listener->Notify(appName, replacesId, icon, summary, body, {}, hints, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    QVERIFY(d->getLastPackage()->hasPayload());
    buffer = dynamic_cast<QBuffer*>(d->getLastPackage()->payload().data());
    QCOMPARE(d->getLastPackage()->payloadSize(), buffer->size());
    // image-data is attached as png data
    QVERIFY(image.loadFromData(reinterpret_cast<const uchar*>(buffer->data().constData()), buffer->size(), "PNG"));
    // image_data has priority over image_path/image-path:
    QCOMPARE(image.byteCount(), rowStride*height);
    // rgba -> argb conversion was done correctly:
    COMPARE_PIXEL(0,0);
    COMPARE_PIXEL(1,0);
    COMPARE_PIXEL(0,1);
    COMPARE_PIXEL(1,1);

    // same for icon_data, which has lowest priority
    hints.clear();
    hints.insert(QStringLiteral("icon_data"), imageData);
    retId = listener->Notify(appName, replacesId, QLatin1String(""), summary, body, {}, hints, 0);
    QCOMPARE(retId, replacesId);
    QCOMPARE(++proxiedNotifications, d->getSentPackages());
    QVERIFY(d->getLastPackage());
    QVERIFY(d->getLastPackage()->hasPayload());
    buffer = dynamic_cast<QBuffer*>(d->getLastPackage()->payload().data());
    // image-data is attached as png data
    QVERIFY(image.loadFromData(reinterpret_cast<const uchar*>(buffer->data().constData()), buffer->size(), "PNG"));
    QCOMPARE(image.byteCount(), rowStride*height);
    // rgba -> argb conversion was done correctly:
    COMPARE_PIXEL(0,0);
    COMPARE_PIXEL(1,0);
    COMPARE_PIXEL(0,1);
    COMPARE_PIXEL(1,1);

#undef COMPARE_PIXEL
}


QTEST_GUILESS_MAIN(TestNotificationListener);

#include "testnotificationlistener.moc"
