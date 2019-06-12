/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 * Copyright 2018 Simon Redman <simon@ergotech.com>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "telephonyplugin.h"

#include <KLocalizedString>
#include <QDebug>
#include <QDBusReply>

#include <KPluginFactory>
#include <KNotification>

K_PLUGIN_CLASS_WITH_JSON(TelephonyPlugin, "kdeconnect_telephony.json")

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_TELEPHONY, "kdeconnect.plugin.telephony")

TelephonyPlugin::TelephonyPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

TelephonyPlugin::~TelephonyPlugin()
{
}

KNotification* TelephonyPlugin::createNotification(const NetworkPacket& np)
{
    const QString event = np.get<QString>(QStringLiteral("event"));
    const QString phoneNumber = np.get<QString>(QStringLiteral("phoneNumber"), i18n("unknown number"));
    const QString contactName = np.get<QString>(QStringLiteral("contactName"), phoneNumber);
    const QByteArray phoneThumbnail = QByteArray::fromBase64(np.get<QByteArray>(QStringLiteral("phoneThumbnail"), ""));

    QString content, type, icon;
    KNotification::NotificationFlags flags = KNotification::CloseOnTimeout;

    const QString title = device()->name();

    if (event == QLatin1String("ringing")) {
        type = QStringLiteral("callReceived");
        icon = QStringLiteral("call-start");
        content = i18n("Incoming call from %1", contactName);
    } else if (event == QLatin1String("missedCall")) {
        type = QStringLiteral("missedCall");
        icon = QStringLiteral("call-start");
        content = i18n("Missed call from %1", contactName);
        flags |= KNotification::Persistent; //Note that in Unity this generates a message box!
    } else if (event == QLatin1String("talking")) {
        return nullptr;
    } else {
#ifndef NDEBUG
        return nullptr;
#else
        type = QStringLiteral("callReceived");
        icon = QStringLiteral("phone");
        content = i18n("Unknown telephony event: %1", event);
#endif
    }

    Q_EMIT callReceived(type, phoneNumber, contactName);

    qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Creating notification with type:" << type;

    KNotification* notification = new KNotification(type, flags, this);
    if (!phoneThumbnail.isEmpty()) {
        QPixmap photo;
        photo.loadFromData(phoneThumbnail, "JPEG");
        notification->setPixmap(photo);
    } else {
        notification->setIconName(icon);
    }
    notification->setComponentName(QStringLiteral("kdeconnect"));
    notification->setTitle(title);
    notification->setText(content);

    if (event == QLatin1String("ringing")) {
        notification->setActions( QStringList(i18n("Mute Call")) );
        connect(notification, &KNotification::action1Activated, this, &TelephonyPlugin::sendMutePacket);
    }
    return notification;
}

bool TelephonyPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.get<bool>(QStringLiteral("isCancel"))) {

        //TODO: Clear the old notification
        return true;
    }

    const QString& event = np.get<QString>(QStringLiteral("event"), QStringLiteral("unknown"));

    // Handle old-style packets
    if (np.type() == PACKET_TYPE_TELEPHONY) {
        if (event == QLatin1String("sms")) {
            return false;
        }
        KNotification* n = createNotification(np);
        if (n != nullptr) n->sendEvent();
        return true;
    }

    return true;
}

void TelephonyPlugin::sendMutePacket()
{
    NetworkPacket packet(PACKET_TYPE_TELEPHONY_REQUEST_MUTE, {{QStringLiteral("action"), QStringLiteral("mute")}});
    sendPacket(packet);
}

QString TelephonyPlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/telephony");
}

#include "telephonyplugin.moc"
