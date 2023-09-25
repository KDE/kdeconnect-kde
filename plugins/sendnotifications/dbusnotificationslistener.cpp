/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 * SPDX-FileCopyrightText: 2018 Richard Liebscher <richard.liebscher@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dbusnotificationslistener.h"

#include <limits>

#include <QBuffer>
#include <QFile>
#include <QImage>

#include <kiconloader.h>
#include <kicontheme.h>

#include "plugin_sendnotifications_debug.h"
#include <core/kdeconnectplugin.h>
#include <core/kdeconnectpluginconfig.h>

namespace
{
// https://specifications.freedesktop.org/notification-spec/notification-spec-latest.html
inline constexpr const char *NOTIFY_SIGNATURE = "susssasa{sv}i";
inline constexpr const char *IMAGE_DATA_SIGNATURE = "iiibiiay";

QString becomeMonitor(DBusConnection *conn, const char *match)
{
    // message
    DBusMessage *msg = dbus_message_new_method_call(DBUS_SERVICE_DBUS, DBUS_PATH_DBUS, DBUS_INTERFACE_MONITORING, "BecomeMonitor");
    Q_ASSERT(msg != nullptr);

    // arguments
    const char *matches[] = {match};
    const char **matches_ = matches;
    dbus_uint32_t flags = 0;

    bool success = dbus_message_append_args(msg, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &matches_, 1, DBUS_TYPE_UINT32, &flags, DBUS_TYPE_INVALID);
    if (!success) {
        dbus_message_unref(msg);
        return QStringLiteral("Failed to call dbus_message_append_args");
    }

    // send
    // TODO: wait and check for error: dbus_connection_send_with_reply_and_block
    success = dbus_connection_send(conn, msg, nullptr);
    if (!success) {
        dbus_message_unref(msg);
        return QStringLiteral("Failed to call dbus_connection_send");
    }

    dbus_message_unref(msg);

    return QString();
}

extern "C" DBusHandlerResult handleMessageFromC(DBusConnection *, DBusMessage *message, void *user_data)
{
    auto *self = static_cast<DBusNotificationsListenerThread *>(user_data);
    if (dbus_message_is_method_call(message, "org.freedesktop.Notifications", "Notify")) {
        self->handleNotifyCall(message);
    }
    // Monitors must not allow libdbus to reply to messages, so we eat the message.
    return DBUS_HANDLER_RESULT_HANDLED;
}

unsigned nextUnsigned(DBusMessageIter *iter)
{
    Q_ASSERT(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_UINT32);
    DBusBasicValue value;
    dbus_message_iter_get_basic(iter, &value);
    dbus_message_iter_next(iter);
    return value.u32;
}

int nextInt(DBusMessageIter *iter)
{
    Q_ASSERT(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_INT32);
    DBusBasicValue value;
    dbus_message_iter_get_basic(iter, &value);
    dbus_message_iter_next(iter);
    return value.i32;
}

QString nextString(DBusMessageIter *iter)
{
    Q_ASSERT(dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING);
    DBusBasicValue value;
    dbus_message_iter_get_basic(iter, &value);
    dbus_message_iter_next(iter);
    return QString::fromUtf8(value.str);
}

QStringList nextStringList(DBusMessageIter *iter)
{
    DBusMessageIter sub;
    dbus_message_iter_recurse(iter, &sub);
    dbus_message_iter_next(iter);

    QStringList list;
    while (dbus_message_iter_get_arg_type(&sub) != DBUS_TYPE_INVALID) {
        list.append(nextString(&sub));
    }
    return list;
}

QVariant nextVariant(DBusMessageIter *iter)
{
    int type = dbus_message_iter_get_arg_type(iter);
    if (type != DBUS_TYPE_VARIANT) {
        return QVariant();
    }

    DBusMessageIter sub;
    dbus_message_iter_recurse(iter, &sub);
    dbus_message_iter_next(iter);

    type = dbus_message_iter_get_arg_type(&sub);
    if (dbus_type_is_basic(type)) {
        DBusBasicValue value;
        dbus_message_iter_get_basic(&sub, &value);
        switch (type) {
        case DBUS_TYPE_BOOLEAN:
            return QVariant(value.bool_val);
        case DBUS_TYPE_INT16:
            return QVariant(value.i16);
        case DBUS_TYPE_INT32:
            return QVariant(value.i32);
        case DBUS_TYPE_INT64:
            return QVariant((qlonglong)value.i64);
        case DBUS_TYPE_UINT16:
            return QVariant(value.u16);
        case DBUS_TYPE_UINT32:
            return QVariant(value.u32);
        case DBUS_TYPE_UINT64:
            return QVariant((qulonglong)value.u64);
        case DBUS_TYPE_BYTE:
            return QVariant(value.byt);
        case DBUS_TYPE_DOUBLE:
            return QVariant(value.dbl);
        case DBUS_TYPE_STRING:
            return QVariant(QString::fromUtf8(value.str));
        case DBUS_STRUCT_BEGIN_CHAR: {
        }
        default:
            break;
        }
    }

    qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Unimplemented conversation of type" << QChar(type) << type;

    return QVariant();
}

static QVariantMap nextVariantMap(DBusMessageIter *iter)
{
    DBusMessageIter sub;
    dbus_message_iter_recurse(iter, &sub);
    dbus_message_iter_next(iter);

    QVariantMap map;
    while (dbus_message_iter_get_arg_type(&sub) != DBUS_TYPE_INVALID) {
        DBusMessageIter entry;
        dbus_message_iter_recurse(&sub, &entry);
        dbus_message_iter_next(&sub);
        QString key = nextString(&entry);
        QVariant value = nextVariant(&entry);
        map.insert(key, value);
    }
    return map;
}
}

void DBusNotificationsListenerThread::run()
{
    DBusError err = DBUS_ERROR_INIT;
    m_connection = dbus_bus_get_private(DBUS_BUS_SESSION, &err);
    if (dbus_error_is_set(&err)) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "D-Bus connection failed" << err.message;
        dbus_error_free(&err);
        return;
    }

    Q_ASSERT(m_connection != nullptr);

    dbus_connection_set_route_peer_messages(m_connection, true);
    dbus_connection_set_exit_on_disconnect(m_connection, false);
    dbus_connection_add_filter(m_connection, handleMessageFromC, this, nullptr);

    QString error = becomeMonitor(m_connection,
                                  "interface='org.freedesktop.Notifications',"
                                  "member='Notify'");

    if (!error.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS).noquote() << "Failed to become a DBus monitor."
                                                                 << "No notifictions will be sent. Error:" << error;
    }

    // wake up every minute to see if we are still connected
    while (m_connection != nullptr) {
        dbus_connection_read_write_dispatch(m_connection, 60 * 1000);
    }

    deleteLater();
}

void DBusNotificationsListenerThread::stop()
{
    if (m_connection) {
        dbus_connection_close(m_connection);
        dbus_connection_unref(m_connection);
        m_connection = nullptr;
    }
}

void DBusNotificationsListenerThread::handleNotifyCall(DBusMessage *message)
{
    DBusMessageIter iter;
    dbus_message_iter_init(message, &iter);

    if (!dbus_message_has_signature(message, NOTIFY_SIGNATURE)) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS).nospace()
            << "Call to Notify has wrong signature. Expected " << NOTIFY_SIGNATURE << ", got " << dbus_message_get_signature(message);
        return;
    }

    QString appName = nextString(&iter);
    uint replacesId = nextUnsigned(&iter);
    QString appIcon = nextString(&iter);
    QString summary = nextString(&iter);
    QString body = nextString(&iter);
    QStringList actions = nextStringList(&iter);
    QVariantMap hints = nextVariantMap(&iter);
    int timeout = nextInt(&iter);

    Q_EMIT notificationReceived(appName, replacesId, appIcon, summary, body, actions, hints, timeout);
}

DBusNotificationsListener::DBusNotificationsListener(KdeConnectPlugin *aPlugin)
    : NotificationsListener(aPlugin)
    , m_thread(new DBusNotificationsListenerThread)
{
    connect(m_thread, &DBusNotificationsListenerThread::notificationReceived, this, &DBusNotificationsListener::onNotify);
    m_thread->start();
}

DBusNotificationsListener::~DBusNotificationsListener()
{
    m_thread->stop();
    m_thread->quit();
}

void DBusNotificationsListener::onNotify(const QString &appName,
                                         uint replacesId,
                                         const QString &appIcon,
                                         const QString &summary,
                                         const QString &body,
                                         const QStringList &actions,
                                         const QVariantMap &hints,
                                         int timeout)
{
    Q_UNUSED(actions);

    // qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Got notification appName=" << appName << "replacesId=" << replacesId
    // << "appIcon=" << appIcon << "summary=" << summary << "body=" << body << "actions=" << actions << "hints=" << hints << "timeout=" << timeout;

    auto *config = m_plugin->config();
    if (timeout > 0 && config->getBool(QStringLiteral("generalPersistent"), false)) {
        return;
    }

    if (!checkApplicationName(appName, appIcon)) {
        return;
    }

    int urgency = -1;
    auto urgencyHint = hints.constFind(QStringLiteral("urgency"));
    if (urgencyHint != hints.cend()) {
        bool ok = false;
        urgency = urgencyHint->toInt(&ok);
        if (!ok) {
            urgency = -1;
        }
    }
    if (urgency > -1 && urgency < config->getInt(QStringLiteral("generalUrgency"), 0)) {
        return;
    }

    if (summary.isEmpty()) {
        return;
    }

    const bool includeBody = config->getBool(QStringLiteral("generalIncludeBody"), true);

    QString ticker = summary;
    if (!body.isEmpty() && includeBody) {
        ticker += QLatin1String(": ") + body;
    }

    if (checkIsInBlacklist(appName, ticker)) {
        return;
    }

    // qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Sending notification from" << appName << ":" <<ticker << "; appIcon=" << appIcon;

    static unsigned id = 0;
    if (id == std::numeric_limits<unsigned>::max()) {
        id = 0;
    }
    NetworkPacket np(PACKET_TYPE_NOTIFICATION,
                     {
                         {QStringLiteral("id"), replacesId > 0 ? replacesId : id++},
                         {QStringLiteral("appName"), appName},
                         {QStringLiteral("ticker"), ticker},
                         {QStringLiteral("isClearable"), timeout == -1},
                         {QStringLiteral("title"), summary},
                         {QStringLiteral("silent"), false},
                     });

    if (!body.isEmpty() && includeBody) {
        np.set(QStringLiteral("text"), body);
    }

    // Only send icon on first notify (replacesId == 0)
    if (config->getBool(QStringLiteral("generalSynchronizeIcons"), true) && replacesId == 0) {
        QSharedPointer<QIODevice> iconSource;
        // try different image sources according to priorities in notifications-spec version 1.2:
        auto it = hints.constFind(QStringLiteral("image-data"));
        if (it != hints.cend() || (it = hints.constFind(QStringLiteral("image_data"))) != hints.cend()) {
            iconSource = iconForImageData(it.value());
        } else if ((it = hints.constFind(QStringLiteral("image-path"))) != hints.cend()
                   || (it = hints.constFind(QStringLiteral("image_path"))) != hints.cend()) {
            iconSource = iconForIconName(it.value().toString());
        } else if (!appIcon.isEmpty()) {
            iconSource = iconForIconName(appIcon);
        } else if ((it = hints.constFind(QStringLiteral("icon_data"))) != hints.cend()) {
            iconSource = iconForImageData(it.value());
        }
        if (iconSource) {
            np.setPayload(iconSource, iconSource->size());
        }
    }

    m_plugin->sendPacket(np);
}

bool DBusNotificationsListener::parseImageDataArgument(const QVariant &argument,
                                                       int &width,
                                                       int &height,
                                                       int &rowStride,
                                                       int &bitsPerSample,
                                                       int &channels,
                                                       bool &hasAlpha,
                                                       QByteArray &imageData) const
{
    // FIXME
    // if (!argument.canConvert<QDBusArgument>()) {
    //     return false;
    // }
    // const QDBusArgument dbusArg = argument.value<QDBusArgument>();
    // dbusArg.beginStructure();
    // dbusArg >> width >> height >> rowStride >> hasAlpha >> bitsPerSample >> channels >> imageData;
    // dbusArg.endStructure();
    return true;
}

QSharedPointer<QIODevice> DBusNotificationsListener::iconForImageData(const QVariant &argument) const
{
    int width, height, rowStride, bitsPerSample, channels;
    bool hasAlpha;
    QByteArray imageData;

    if (!parseImageDataArgument(argument, width, height, rowStride, bitsPerSample, channels, hasAlpha, imageData))
        return QSharedPointer<QIODevice>();

    if (bitsPerSample != 8) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Unsupported image format:"
                                                       << "width=" << width << "height=" << height << "rowStride=" << rowStride
                                                       << "bitsPerSample=" << bitsPerSample << "channels=" << channels << "hasAlpha=" << hasAlpha;
        return QSharedPointer<QIODevice>();
    }

    QImage image(reinterpret_cast<uchar *>(imageData.data()), width, height, rowStride, hasAlpha ? QImage::Format_ARGB32 : QImage::Format_RGB32);
    if (hasAlpha) {
        image = std::move(image).rgbSwapped(); // RGBA --> ARGB
    }

    QSharedPointer<QIODevice> buffer = iconFromQImage(image);
    if (!buffer) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Could not initialize image buffer";
        return QSharedPointer<QIODevice>();
    }

    return buffer;
}

QSharedPointer<QIODevice> DBusNotificationsListener::iconForIconName(const QString &iconName) const
{
    int size = KIconLoader::SizeHuge; // use big size to allow for good quality on high-DPI mobile devices
    QString iconPath = iconName;
    if (!QFile::exists(iconName)) {
        const KIconTheme *iconTheme = KIconLoader::global()->theme();
        if (iconTheme) {
            iconPath = iconTheme->iconPath(iconName + QLatin1String(".png"), size, KIconLoader::MatchBest);
            if (iconPath.isEmpty()) {
                iconPath = iconTheme->iconPath(iconName + QLatin1String(".svg"), size, KIconLoader::MatchBest);
                if (iconPath.isEmpty()) {
                    iconPath = iconTheme->iconPath(iconName + QLatin1String(".svgz"), size, KIconLoader::MatchBest);
                }
            }
        } else {
            qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "KIconLoader has no theme set";
        }
    }
    if (iconPath.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Could not find notification icon:" << iconName;
        return QSharedPointer<QIODevice>();
    } else if (iconPath.endsWith(QLatin1String(".png"))) {
        return QSharedPointer<QIODevice>(new QFile(iconPath));
    } else {
        // TODO: cache icons
        return iconFromQImage(QImage(iconPath));
    }
}

#include "moc_dbusnotificationslistener.cpp"
