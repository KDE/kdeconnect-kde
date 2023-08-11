/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 * SPDX-FileCopyrightText: 2018 Richard Liebscher <richard.liebscher@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "notificationslistener.h"

#include <QBuffer>
#include <QDBusArgument>
#include <QFile>
#include <QIODevice>
#include <QImage>
#include <QDBusMessage>
#include <QDBusConnection>
#include <QDBusConnectionInterface>

#include <core/kdeconnectplugin.h>
#include <core/kdeconnectpluginconfig.h>

#include <kiconloader.h>
#include <kicontheme.h>

#include "notifyingapplication.h"
#include "plugin_sendnotifications_debug.h"
#include "sendnotificationsplugin.h"

void NotificationsListener::handleMessage(const QDBusMessage& message)
{
    qWarning() << message;

    if (message.interface() != QStringLiteral("org.freedesktop.Notifications") || message.member() != QStringLiteral("Notify")) {
        qWarning() << "Wrong method";
        return;
    }

    const QString notifySignature = QStringLiteral("susssasa{sv}i");

    if (message.signature() != notifySignature) {
        qWarning() << "Wrong signature";
    }

    QList<QVariant> args = message.arguments();
    if (args.isEmpty()) {
        return;
    }

    QString appName = args.at(0).toString();
    uint replacesId = args.at(1).toUInt();
    QString appIcon = args.at(2).toString();
    QString summary = args.at(3).toString();
    QString body = args.at(4).toString();
    QStringList actions = args.at(5).toStringList();
    QVariantMap hints = args.at(6).toMap();
    int timeout = args.at(7).toInt();

    handleNotification(appName, replacesId, appIcon, summary, body, actions, hints, timeout);
}

NotificationsListener::NotificationsListener(KdeConnectPlugin *aPlugin)
    : QObject(aPlugin)
    , m_plugin(aPlugin)
    , sessionBus(QDBusConnection::sessionBus())
{
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    qRegisterMetaTypeStreamOperators<NotifyingApplication>("NotifyingApplication");
#endif

    if (!sessionBus.isConnected()) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "D-Bus connection failed";
        return;
    }

    loadApplications();

    connect(m_plugin->config(), &KdeConnectPluginConfig::configChanged, this, &NotificationsListener::loadApplications);

    const QString dbusServiceBus = QStringLiteral("org.freedesktop.DBus");
    const QString dbusPathDbus = QStringLiteral("/org/freedesktop/DBus");
    const QString dbusInterfaceMonitoring = QStringLiteral("org.freedesktop.DBus.Monitoring");
    const QString becomeMonitor = QStringLiteral("BecomeMonitor");
    const quint32 flags = 0;
    const QString match = QStringLiteral("interface='org.freedesktop.Notifications',member='Notify'");

    QObject::connect(sessionBus.interface(), SIGNAL(MessageReceived(QDBusMessage)), this, SLOT(handleMessage(QDBusMessage)));

    QDBusMessage msg = QDBusMessage::createMethodCall(dbusServiceBus, dbusPathDbus, dbusInterfaceMonitoring, becomeMonitor);
    QStringList matches = {match};
    msg.setArguments(QList<QVariant>{QVariant(matches), QVariant(flags)});
    QDBusMessage reply = sessionBus.call(msg);
    if (reply.type() == QDBusMessage::ErrorMessage) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS).noquote() << "Failed to become a DBus monitor."
                                                                 << "No notifictions will be sent. Error:" << reply.errorMessage();
    }
}

void NotificationsListener::loadApplications()
{
    m_applications.clear();
    const QVariantList list = m_plugin->config()->getList(QStringLiteral("applications"));
    for (const auto &a : list) {
        NotifyingApplication app = a.value<NotifyingApplication>();
        if (!m_applications.contains(app.name))
            m_applications.insert(app.name, app);
    }
    // qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Loaded" << m_applications.size() << " applications";
}

bool NotificationsListener::parseImageDataArgument(const QVariant &argument,
                                                   int &width,
                                                   int &height,
                                                   int &rowStride,
                                                   int &bitsPerSample,
                                                   int &channels,
                                                   bool &hasAlpha,
                                                   QByteArray &imageData) const
{
    if (!argument.canConvert<QDBusArgument>())
        return false;
    const QDBusArgument dbusArg = argument.value<QDBusArgument>();
    dbusArg.beginStructure();
    dbusArg >> width >> height >> rowStride >> hasAlpha >> bitsPerSample >> channels >> imageData;
    dbusArg.endStructure();
    return true;
}

QSharedPointer<QIODevice> NotificationsListener::iconFromQImage(const QImage &image) const
{
    QSharedPointer<QBuffer> buffer = QSharedPointer<QBuffer>(new QBuffer);
    if (!buffer->open(QIODevice::WriteOnly) && !image.save(buffer.data(), "PNG")) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Could not initialize image buffer";
        return QSharedPointer<QIODevice>();
    }
    return buffer;
}

QSharedPointer<QIODevice> NotificationsListener::iconForImageData(const QVariant &argument) const
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
        image = image.rgbSwapped(); // RGBA --> ARGB
    }

    QSharedPointer<QIODevice> buffer = iconFromQImage(image);
    if (!buffer) {
        qCWarning(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Could not initialize image buffer";
        return QSharedPointer<QIODevice>();
    }

    return buffer;
}

QSharedPointer<QIODevice> NotificationsListener::iconForIconName(const QString &iconName) const
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

void NotificationsListener::handleNotification(const QString &appName,
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

    NotifyingApplication app;
    if (!m_applications.contains(appName)) {
        // new application -> add to config
        app.name = appName;
        app.icon = appIcon;
        app.active = true;
        app.blacklistExpression = QRegularExpression();
        m_applications.insert(app.name, app);
        // update config
        QVariantList list;
        for (const auto &a : std::as_const(m_applications))
            list << QVariant::fromValue<NotifyingApplication>(a);
        config->setList(QStringLiteral("applications"), list);
        // qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Added new application to config:" << app;
    } else {
        app = m_applications.value(appName);
    }

    if (!app.active) {
        return;
    }

    if (timeout > 0 && config->getBool(QStringLiteral("generalPersistent"), false)) {
        return;
    }

    int urgency = -1;
    auto urgencyHint = hints.find(QStringLiteral("urgency"));
    if (urgencyHint != hints.end()) {
        bool ok;
        urgency = urgencyHint->toInt(&ok);
        if (!ok)
            urgency = -1;
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
        ticker += QStringLiteral(": ") + body;
    }

    if (app.blacklistExpression.isValid() && !app.blacklistExpression.pattern().isEmpty() && app.blacklistExpression.match(ticker).hasMatch()) {
        return;
    }

    // qCDebug(KDECONNECT_PLUGIN_SENDNOTIFICATIONS) << "Sending notification from" << appName << ":" <<ticker << "; appIcon=" << appIcon;

    bool silent = false;
    static unsigned id = 0;
    NetworkPacket np(PACKET_TYPE_NOTIFICATION,
                     {
                         {QStringLiteral("id"), QString::number(replacesId > 0 ? replacesId : ++id)},
                         {QStringLiteral("appName"), appName},
                         {QStringLiteral("ticker"), ticker},
                         {QStringLiteral("isClearable"), timeout == -1},
                         {QStringLiteral("title"), summary},
                         {QStringLiteral("silent"), silent},
                     });

    if (!body.isEmpty() && includeBody) {
        np.set(QStringLiteral("text"), body);
    }

    // Only send icon on first notify (replacesId == 0)
    if (config->getBool(QStringLiteral("generalSynchronizeIcons"), true) && replacesId == 0) {
        QSharedPointer<QIODevice> iconSource;
        // try different image sources according to priorities in notifications-spec version 1.2:
        auto it = hints.find(QStringLiteral("image-data"));
        if (it != hints.end() || (it = hints.find(QStringLiteral("image_data"))) != hints.end()) {
            iconSource = iconForImageData(it.value());
        } else if ((it = hints.find(QStringLiteral("image-path"))) != hints.end() || (it = hints.find(QStringLiteral("image_path"))) != hints.end()) {
            iconSource = iconForIconName(it.value().toString());
        } else if (!appIcon.isEmpty()) {
            iconSource = iconForIconName(appIcon);
        } else if ((it = hints.find(QStringLiteral("icon_data"))) != hints.end()) {
            iconSource = iconForImageData(it.value());
        }
        if (iconSource) {
            np.setPayload(iconSource, iconSource->size());
        }
    }

    m_plugin->sendPacket(np);
}

#include "moc_notificationslistener.cpp"
