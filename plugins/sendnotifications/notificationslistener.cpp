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

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusArgument>
#include <QtDebug>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QImage>
#include <KConfig>
#include <KConfigGroup>

#include <kiconloader.h>
#include <kicontheme.h>

#include <core/device.h>
#include <core/kdeconnectplugin.h>

#include "notificationslistener.h"
#include "sendnotificationsplugin.h"
#include "sendnotification_debug.h"
#include "notifyingapplication.h"

NotificationsListener::NotificationsListener(KdeConnectPlugin* aPlugin)
    : QDBusAbstractAdaptor(aPlugin),
      mPlugin(aPlugin)
{
    qRegisterMetaTypeStreamOperators<NotifyingApplication>("NotifyingApplication");

    bool ret = QDBusConnection::sessionBus()
                .registerObject("/org/freedesktop/Notifications",
                                this,
                                QDBusConnection::ExportScriptableContents);
    if (!ret)
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATION)
                << "Error registering notifications listener for device"
                << mPlugin->device()->name() << ":"
                << QDBusConnection::sessionBus().lastError();
    else
        qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION)
                << "Registered notifications listener for device"
                << mPlugin->device()->name();

    QDBusInterface iface("org.freedesktop.DBus", "/org/freedesktop/DBus",
                         "org.freedesktop.DBus");
    iface.call("AddMatch",
               "interface='org.freedesktop.Notifications',member='Notify',type='method_call',eavesdrop='true'");

    setTranslatedAppName();
    loadApplications();

    connect(mPlugin->config(), &KdeConnectPluginConfig::configChanged, this, &NotificationsListener::loadApplications);
}

NotificationsListener::~NotificationsListener()
{
    qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Destroying NotificationsListener";
    QDBusInterface iface("org.freedesktop.DBus", "/org/freedesktop/DBus",
                         "org.freedesktop.DBus");
    QDBusMessage res = iface.call("RemoveMatch",
                                  "interface='org.freedesktop.Notifications',member='Notify',type='method_call',eavesdrop='true'");
    QDBusConnection::sessionBus().unregisterObject("/org/freedesktop/Notifications");
}

void NotificationsListener::setTranslatedAppName()
{
    QString filePath = QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("knotifications5/kdeconnect.notifyrc"), QStandardPaths::LocateFile);
    if (filePath.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Couldn't find kdeconnect.notifyrc to hide kdeconnect notifications on the devices. Using default name.";
        mTranslatedAppName = QStringLiteral("KDE Connect");
        return;
    }

    KConfig config(filePath, KConfig::OpenFlag::SimpleConfig);
    KConfigGroup globalgroup(&config, QStringLiteral("Global"));
    mTranslatedAppName = globalgroup.readEntry(QStringLiteral("Name"), QStringLiteral("KDE Connect"));
}

void NotificationsListener::loadApplications()
{
    applications.clear();
    QVariantList list = mPlugin->config()->getList("applications");
    Q_FOREACH (const auto& a, list) {
        NotifyingApplication app = a.value<NotifyingApplication>();
        if (!applications.contains(app.name))
            applications.insert(app.name, app);
    }
    //qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Loaded" << applications.size() << " applications";
}

bool NotificationsListener::parseImageDataArgument(const QVariant& argument,
                                                   int& width, int& height,
                                                   int& rowStride, int& bitsPerSample,
                                                   int& channels, bool& hasAlpha,
                                                   QByteArray& imageData) const
{
    if (!argument.canConvert<QDBusArgument>())
        return false;
    const QDBusArgument dbusArg = argument.value<QDBusArgument>();
    dbusArg.beginStructure();
    dbusArg >> width >> height >> rowStride >> hasAlpha >> bitsPerSample
            >> channels  >> imageData;
    dbusArg.endStructure();
    return true;
}

QSharedPointer<QIODevice> NotificationsListener::iconForImageData(const QVariant& argument) const
{
    int width, height, rowStride, bitsPerSample, channels;
    bool hasAlpha;
    QByteArray imageData;

    if (!parseImageDataArgument(argument, width, height, rowStride, bitsPerSample,
                                channels, hasAlpha, imageData))
        return QSharedPointer<QIODevice>();

    if (bitsPerSample != 8) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Unsupported image format:"
                                                      << "width=" << width
                                                      << "height=" << height
                                                      << "rowStride=" << rowStride
                                                      << "bitsPerSample=" << bitsPerSample
                                                      << "channels=" << channels
                                                      << "hasAlpha=" << hasAlpha;
        return QSharedPointer<QIODevice>();
    }

    QImage image(reinterpret_cast<uchar*>(imageData.data()), width, height, rowStride,
                 hasAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    if (hasAlpha)
        image = image.rgbSwapped();  // RGBA --> ARGB

    QSharedPointer<QBuffer> buffer = QSharedPointer<QBuffer>(new QBuffer);
    if (!buffer || !buffer->open(QIODevice::WriteOnly) ||
            !image.save(buffer.data(), "PNG")) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Could not initialize image buffer";
        return QSharedPointer<QIODevice>();
    }

    return buffer;
}

QSharedPointer<QIODevice> NotificationsListener::iconForIconName(const QString &iconName) const
{
    int size = KIconLoader::SizeEnormous;  // use big size to allow for good
                                           // quality on high-DPI mobile devices
    QString iconPath = KIconLoader::global()->iconPath(iconName, -size, true);
    if (!iconPath.isEmpty()) {
        if (!iconPath.endsWith(QLatin1String(".png")) &&
                KIconLoader::global()->theme()->name() != QLatin1String("hicolor")) {
            // try falling back to hicolor theme:
            KIconTheme hicolor(QStringLiteral("hicolor"));
            if (hicolor.isValid()) {
                iconPath = hicolor.iconPath(iconName + ".png", size, KIconLoader::MatchBest);
                //qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Found non-png icon in default theme trying fallback to hicolor:" << iconPath;
            }
        }
    }

    if (iconPath.endsWith(QLatin1String(".png")))
        return QSharedPointer<QIODevice>(new QFile(iconPath));
    return QSharedPointer<QIODevice>();
}
uint NotificationsListener::Notify(const QString &appName, uint replacesId,
                                   const QString &appIcon,
                                   const QString &summary, const QString &body,
                                   const QStringList &actions,
                                   const QVariantMap &hints, int timeout)
{
    static int id = 0;
    Q_UNUSED(actions);

    //qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Got notification appName=" << appName << "replacesId=" << replacesId << "appIcon=" << appIcon << "summary=" << summary << "body=" << body << "actions=" << actions << "hints=" << hints << "timeout=" << timeout;

    // skip our own notifications
    if (appName == mTranslatedAppName)
        return 0;

    NotifyingApplication app;
    if (!applications.contains(appName)) {
        // new application -> add to config
        app.name = appName;
        app.icon = appIcon;
        app.active = true;
        app.blacklistExpression = QRegularExpression();
        applications.insert(app.name, app);
        // update config:
        QVariantList list;
        Q_FOREACH (const auto& a, applications)
            list << QVariant::fromValue<NotifyingApplication>(a);
        mPlugin->config()->setList("applications", list);
        //qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Added new application to config:" << app;
    } else
        app = applications.value(appName);

    if (!app.active)
        return 0;

    if (timeout > 0 && mPlugin->config()->get("generalPersistent", false))
        return 0;

    int urgency = -1;
    if (hints.contains("urgency")) {
        bool ok;
        urgency = hints["urgency"].toInt(&ok);
        if (!ok)
            urgency = -1;
    }
    if (urgency > -1 && urgency < mPlugin->config()->get<int>("generalUrgency", 0))
        return 0;

    QString ticker = summary;
    if (!body.isEmpty() && mPlugin->config()->get("generalIncludeBody", true))
        ticker += QLatin1String(": ") + body;

    if (app.blacklistExpression.isValid() &&
            !app.blacklistExpression.pattern().isEmpty() &&
            app.blacklistExpression.match(ticker).hasMatch())
        return 0;

    //qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Sending notification from" << appName << ":" <<ticker << "; appIcon=" << appIcon;
    NetworkPackage np(PACKAGE_TYPE_NOTIFICATION, {
        {"id", QString::number(replacesId > 0 ? replacesId : ++id)},
        {"appName", appName},
        {"ticker", ticker},
        {"isClearable", timeout == 0}
    });                                   // KNotifications are persistent if
                                          // timeout == 0, for other notifications
                                          // clearability is pointless

    // sync any icon data?
    if (mPlugin->config()->get("generalSynchronizeIcons", true)) {
        QSharedPointer<QIODevice> iconSource;
        // try different image sources according to priorities in notifications-
        // spec version 1.2:
        if (hints.contains("image-data"))
            iconSource = iconForImageData(hints["image-data"]);
        else if (hints.contains("image_data"))  // 1.1 backward compatibility
            iconSource = iconForImageData(hints["image_data"]);
        else if (hints.contains("image-path"))
            iconSource = iconForIconName(hints["image-path"].toString());
        else if (hints.contains("image_path"))  // 1.1 backward compatibility
            iconSource = iconForIconName(hints["image_path"].toString());
        else if (!appIcon.isEmpty())
            iconSource = iconForIconName(appIcon);
        else if (hints.contains("icon_data"))  // < 1.1 backward compatibility
            iconSource = iconForImageData(hints["icon_data"]);

        if (iconSource)
            np.setPayload(iconSource, iconSource->size());
    }

    mPlugin->sendPackage(np);

    return (replacesId > 0 ? replacesId : id);
}
