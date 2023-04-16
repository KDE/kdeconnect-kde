/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */
#include "notificationslistener.h"

#include <unordered_map>

#include <KConfig>
#include <KConfigGroup>
#include <QDBusArgument>
#include <QDBusInterface>
#include <QImage>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QtDebug>

#include <kiconloader.h>
#include <kicontheme.h>

#include <core/device.h>
#include <core/kdeconnectplugin.h>

#include <dbushelper.h>

#include "notifyingapplication.h"
#include "plugin_sendnotification_debug.h"
#include "sendnotificationsplugin.h"

// In older Qt released, qAsConst isnt available
#include "qtcompat_p.h"

NotificationsListener::NotificationsListener(KdeConnectPlugin *aPlugin)
    : QObject(aPlugin)
    , m_plugin(aPlugin)
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaTypeStreamOperators<NotifyingApplication>("NotifyingApplication");
#endif

    GError *error = nullptr;
    m_gdbusConnection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
    g_assert_no_error(error);
    m_gdbusFilterId = g_dbus_connection_add_filter(m_gdbusConnection, NotificationsListener::onMessageFiltered, this, nullptr);

    g_autoptr(GDBusMessage) msg =
        g_dbus_message_new_method_call("org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus.Monitoring", "BecomeMonitor");

    GVariantBuilder *arrayBuilder = g_variant_builder_new(G_VARIANT_TYPE("as"));
    g_variant_builder_add(arrayBuilder, "s", "interface='org.freedesktop.Notifications'");
    g_variant_builder_add(arrayBuilder, "s", "member='Notify'");

    g_dbus_message_set_body(msg, g_variant_new("(asu)", arrayBuilder, 0u));
    g_dbus_connection_send_message(m_gdbusConnection, msg, GDBusSendMessageFlags::G_DBUS_SEND_MESSAGE_FLAGS_NONE, nullptr, &error);
    g_assert_no_error(error);

    setTranslatedAppName();
    loadApplications();

    connect(m_plugin->config(), &KdeConnectPluginConfig::configChanged, this, &NotificationsListener::loadApplications);
}

NotificationsListener::~NotificationsListener()
{
    qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Destroying NotificationsListener";
    g_dbus_connection_remove_filter(m_gdbusConnection, m_gdbusFilterId);
    g_object_unref(m_gdbusConnection);
}

void NotificationsListener::setTranslatedAppName()
{
    QString filePath =
        QStandardPaths::locate(QStandardPaths::GenericDataLocation, QStringLiteral("knotifications5/kdeconnect.notifyrc"), QStandardPaths::LocateFile);
    if (filePath.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Couldn't find kdeconnect.notifyrc to hide kdeconnect notifications on the devices. Using default name.";
        m_translatedAppName = QStringLiteral("KDE Connect");
        return;
    }

    KConfig config(filePath, KConfig::OpenFlag::SimpleConfig);
    KConfigGroup globalgroup(&config, QStringLiteral("Global"));
    m_translatedAppName = globalgroup.readEntry(QStringLiteral("Name"), QStringLiteral("KDE Connect"));
}

void NotificationsListener::loadApplications()
{
    m_applications.clear();
    const QVariantList list = m_plugin->config()->getList(QStringLiteral("applications"));
    for (const auto &a : list) {
        NotifyingApplication app = a.value<NotifyingApplication>();
        if (!m_applications.contains(app.name)) {
            m_applications.insert(app.name, app);
        }
    }
    // qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Loaded" << applications.size() << " applications";
}

bool NotificationsListener::parseImageDataArgument(GVariant *argument,
                                                   int &width,
                                                   int &height,
                                                   int &rowStride,
                                                   int &bitsPerSample,
                                                   int &channels,
                                                   bool &hasAlpha,
                                                   QByteArray &imageData) const
{
    if (g_variant_n_children(argument) != 7) {
        return false;
    }

    g_autoptr(GVariant) variant;

    variant = g_variant_get_child_value(argument, 0);
    if (g_variant_is_of_type(variant, G_VARIANT_TYPE_INT32)) {
        return false;
    }
    width = g_variant_get_int32(variant);

    variant = g_variant_get_child_value(argument, 1);
    if (g_variant_is_of_type(variant, G_VARIANT_TYPE_INT32)) {
        return false;
    }
    height = g_variant_get_int32(variant);

    variant = g_variant_get_child_value(argument, 2);
    if (g_variant_is_of_type(variant, G_VARIANT_TYPE_INT32)) {
        return false;
    }
    rowStride = g_variant_get_int32(variant);

    variant = g_variant_get_child_value(argument, 3);
    if (g_variant_is_of_type(variant, G_VARIANT_TYPE_BOOLEAN)) {
        return false;
    }
    hasAlpha = g_variant_get_boolean(variant);

    variant = g_variant_get_child_value(argument, 4);
    if (g_variant_is_of_type(variant, G_VARIANT_TYPE_INT32)) {
        return false;
    }
    bitsPerSample = g_variant_get_int32(variant);

    variant = g_variant_get_child_value(argument, 5);
    if (g_variant_is_of_type(variant, G_VARIANT_TYPE_INT32)) {
        return false;
    }
    channels = g_variant_get_int32(variant);

    variant = g_variant_get_child_value(argument, 6);
    if (g_variant_is_of_type(variant, G_VARIANT_TYPE_ARRAY)) {
        return false;
    }
    imageData = g_variant_get_bytestring(variant);

    return true;
}

QSharedPointer<QIODevice> NotificationsListener::iconForImageData(GVariant *argument) const
{
    int width, height, rowStride, bitsPerSample, channels;
    bool hasAlpha;
    QByteArray imageData;

    if (!parseImageDataArgument(argument, width, height, rowStride, bitsPerSample, channels, hasAlpha, imageData))
        return QSharedPointer<QIODevice>();

    if (bitsPerSample != 8) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Unsupported image format:"
                                                      << "width=" << width << "height=" << height << "rowStride=" << rowStride
                                                      << "bitsPerSample=" << bitsPerSample << "channels=" << channels << "hasAlpha=" << hasAlpha;
        return QSharedPointer<QIODevice>();
    }

    QImage image(reinterpret_cast<uchar *>(imageData.data()), width, height, rowStride, hasAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    if (hasAlpha)
        image = image.rgbSwapped(); // RGBA --> ARGB

    QSharedPointer<QBuffer> buffer = QSharedPointer<QBuffer>(new QBuffer);
    if (!buffer || !buffer->open(QIODevice::WriteOnly) || !image.save(buffer.data(), "PNG")) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Could not initialize image buffer";
        return QSharedPointer<QIODevice>();
    }

    return buffer;
}

QSharedPointer<QIODevice> NotificationsListener::iconForIconName(const QString &iconName)
{
    int size = KIconLoader::SizeEnormous; // use big size to allow for good
                                          // quality on high-DPI mobile devices
    QString iconPath = KIconLoader::global()->iconPath(iconName, -size, true);
    if (!iconPath.isEmpty()) {
        if (!iconPath.endsWith(QLatin1String(".png")) && KIconLoader::global()->theme()->name() != QLatin1String("hicolor")) {
            // try falling back to hicolor theme:
            KIconTheme hicolor(QStringLiteral("hicolor"));
            if (hicolor.isValid()) {
                iconPath = hicolor.iconPath(iconName + QStringLiteral(".png"), size, KIconLoader::MatchBest);
                // qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Found non-png icon in default theme trying fallback to hicolor:" << iconPath;
            }
        }
    }

    if (iconPath.endsWith(QLatin1String(".png")))
        return QSharedPointer<QIODevice>(new QFile(iconPath));
    return QSharedPointer<QIODevice>();
}

GDBusMessage *NotificationsListener::onMessageFiltered(GDBusConnection *, GDBusMessage *msg, int, void *parent)
{
    static unsigned id = 0;
    if (!msg) {
        return msg;
    }

    const gchar *interface = g_dbus_message_get_interface(msg);
    if (!interface || strcmp(interface, "org.freedesktop.Notifications")) {
        // The first message will be from org.freedesktop.DBus.Monitoring
        return msg;
    }
    const gchar *member = g_dbus_message_get_member(msg);
    if (!member || strcmp(member, "Notify")) {
        // Even if member is set, the monitor will still notify messages from other members.
        return nullptr;
    }

    g_autoptr(GVariant) bodyVariant = g_dbus_message_get_body(msg);
    Q_ASSERT(bodyVariant);

    // Data order and types: https://specifications.freedesktop.org/notification-spec/notification-spec-latest.html
    g_autoptr(GVariant) variant = g_variant_get_child_value(bodyVariant, 0);
    const QString appName = QString::fromUtf8(g_variant_get_string(variant, nullptr));
    // skip our own notifications
    auto listener = static_cast<NotificationsListener *>(parent);

    if (appName == listener->m_translatedAppName) {
        return nullptr;
    }

    variant = g_variant_get_child_value(bodyVariant, 2);
    const QString appIcon = QString::fromUtf8(g_variant_get_string(variant, nullptr));

    NotifyingApplication app;
    if (!listener->m_applications.contains(appName)) {
        // new application -> add to config
        app.name = appName;
        app.icon = appIcon;
        app.active = true;
        app.blacklistExpression = QRegularExpression();
        listener->m_applications.insert(app.name, app);
        // update config:
        QVariantList list;
        for (const auto &a : std::as_const(listener->m_applications))
            list << QVariant::fromValue<NotifyingApplication>(a);
        listener->m_plugin->config()->setList(QStringLiteral("applications"), list);
        // qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Added new application to config:" << app;
    } else {
        app = listener->m_applications.value(appName);
    }

    if (!app.active) {
        return nullptr;
    }

    variant = g_variant_get_child_value(bodyVariant, 7);
    const auto timeout = g_variant_get_int32(variant);
    if (timeout > 0 && listener->m_plugin->config()->getBool(QStringLiteral("generalPersistent"), false)) {
        return nullptr;
    }

    variant = g_variant_get_child_value(bodyVariant, 6);
    std::unordered_map<QString, GVariant *> hints;
    GVariantIter *iter;
    const gchar *key;
    GVariant *value = nullptr;
    g_variant_get(variant, "a{sv}", &iter);
    while (g_variant_iter_loop(iter, "{sv}", &key, &value)) {
        hints.emplace(QString::fromUtf8(key), value);
    }
    g_variant_iter_free(iter);

    QScopeGuard cleanupHints([&hints] {
        for (auto &[_, hint] : hints) {
            g_variant_unref(hint);
        }
    });

    int urgency = -1;
    if (auto it = hints.find(QStringLiteral("urgency")); it != hints.end()) {
        if (g_variant_is_of_type(it->second, G_VARIANT_TYPE_BYTE)) {
            urgency = g_variant_get_byte(it->second);
        }
    }
    if (urgency > -1 && urgency < listener->m_plugin->config()->getInt(QStringLiteral("generalUrgency"), 0))
        return nullptr;

    variant = g_variant_get_child_value(bodyVariant, 3);
    QString ticker = QString::fromUtf8(g_variant_get_string(variant, nullptr));

    variant = g_variant_get_child_value(bodyVariant, 4);
    const QString body = QString::fromUtf8(g_variant_get_string(variant, nullptr));

    if (!body.isEmpty() && listener->m_plugin->config()->getBool(QStringLiteral("generalIncludeBody"), true)) {
        ticker += QStringLiteral(": ") + body;
    }

    if (app.blacklistExpression.isValid() && !app.blacklistExpression.pattern().isEmpty() && app.blacklistExpression.match(ticker).hasMatch()) {
        return nullptr;
    }

    variant = g_variant_get_child_value(bodyVariant, 1);
    const unsigned replacesId = g_variant_get_uint32(variant);

    // qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Sending notification from" << appName << ":" <<ticker << "; appIcon=" << appIcon;
    NetworkPacket np(PACKET_TYPE_NOTIFICATION,
                     {{QStringLiteral("id"), QString::number(replacesId > 0 ? replacesId : ++id)},
                      {QStringLiteral("appName"), appName},
                      {QStringLiteral("ticker"), ticker},
                      {QStringLiteral("isClearable"), timeout == 0}}); // KNotifications are persistent if
                                                                       // timeout == 0, for other notifications
                                                                       // clearability is pointless

    // sync any icon data?
    if (listener->m_plugin->config()->getBool(QStringLiteral("generalSynchronizeIcons"), true)) {
        QSharedPointer<QIODevice> iconSource;
        // try different image sources according to priorities in notifications-
        // spec version 1.2:
        if (auto it = hints.find(QStringLiteral("image-data")); it != hints.end()) {
            iconSource = listener->iconForImageData(it->second);
        } else if (auto it = hints.find(QStringLiteral("image_data")); it != hints.end()) { // 1.1 backward compatibility
            iconSource = listener->iconForImageData(it->second);
        } else if (auto it = hints.find(QStringLiteral("image-path")); it != hints.end()) {
            iconSource = iconForIconName(QString::fromUtf8(g_variant_get_string(it->second, nullptr)));
        } else if (auto it = hints.find(QStringLiteral("image_path")); it != hints.end()) { // 1.1 backward compatibility
            iconSource = iconForIconName(QString::fromUtf8(g_variant_get_string(it->second, nullptr)));
        } else if (!appIcon.isEmpty()) {
            iconSource = iconForIconName(appIcon);
        } else if (auto it = hints.find(QStringLiteral("icon_data")); it != hints.end()) { // < 1.1 backward compatibility
            iconSource = listener->iconForImageData(it->second);
        }

        if (iconSource)
            np.setPayload(iconSource, iconSource->size());
    }

    listener->m_plugin->sendPacket(np);

    qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATION) << "Got notification appName=" << appName << "replacesId=" << replacesId << "appIcon=" << appIcon
                                                << "summary=" << ticker << "body=" << body << "hints=" << hints.size() << "urgency=" << urgency
                                                << "timeout=" << timeout;

    return nullptr;
}
