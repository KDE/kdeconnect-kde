/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "telephonyplugin.h"

#include <KLocalizedString>
#include <QIcon>
#include <QDebug>

#include <KPluginFactory>


K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< TelephonyPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_TELEPHONY, "kdeconnect.plugin.telephony")

TelephonyPlugin::TelephonyPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{

}

KNotification* TelephonyPlugin::createNotification(const NetworkPackage& np)
{

    const QString event = np.get<QString>("event");
    const QString phoneNumber = np.get<QString>("phoneNumber", i18n("unknown number"));

    QString content, type, icon;
    KNotification::NotificationFlags flags = KNotification::CloseOnTimeout;

    const QString title = device()->name();

    if (event == "ringing") {
        type = QStringLiteral("callReceived");
        icon = QStringLiteral("call-start");
        content = i18n("Incoming call from %1", phoneNumber);
    } else if (event == "missedCall") {
        type = QStringLiteral("missedCall");
        icon = QStringLiteral("call-start");
        content = i18n("Missed call from %1", phoneNumber);
        flags = KNotification::Persistent;
    } else if (event == "sms") {
        type = QStringLiteral("smsReceived");
        icon = QStringLiteral("mail-receive");
        QString messageBody = np.get<QString>("messageBody","");
        content = i18n("SMS from %1<br>%2", phoneNumber, messageBody);
        flags = KNotification::Persistent;
    } else if (event == "talking") {
        return NULL;
    } else {
#ifndef NDEBUG
        return NULL;
#else
        type = QStringLiteral("callReceived");
        icon = QStringLiteral("phone");
        content = i18n("Unknown telephony event: %1", event);
#endif
    }

    qCDebug(KDECONNECT_PLUGIN_TELEPHONY) << "Creating notification with type:" << type;

    KNotification* notification = new KNotification(type, flags, this);
    notification->setIconName(icon);
    notification->setComponentName("kdeconnect");
    notification->setTitle(title);
    notification->setText(content);

    if (event == QLatin1String("ringing")) {
        notification->setActions( QStringList(i18n("Mute Call")) );
        connect(notification, &KNotification::action1Activated, this, &TelephonyPlugin::sendMutePackage);
    }

    return notification;

}

bool TelephonyPlugin::receivePackage(const NetworkPackage& np)
{
    if (np.get<bool>("isCancel")) {

        //It would be awesome to remove the old notification from the system tray here, but there is no way to do it :(
        //Now I realize why at the end of the day I have hundreds of notifications from facebook messages that I HAVE ALREADY READ,
        //...it's just because the telepathy client has no way to remove them! even when it knows that I have read those messages!

    } else {

        KNotification* n = createNotification(np);
        if (n != NULL) n->sendEvent();

    }

    return true;

}

void TelephonyPlugin::sendMutePackage()
{
    NetworkPackage package(PACKAGE_TYPE_TELEPHONY);
    package.set<QString>( "action", "mute" );
    sendPackage(package);
}

#include "telephonyplugin.moc"
