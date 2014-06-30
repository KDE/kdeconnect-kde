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
#include <KIcon>

#include <core/kdebugnamespace.h>

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< TelephonyPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_telephony", "kdeconnect-plugins") )

TelephonyPlugin::TelephonyPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{

}

KNotification* TelephonyPlugin::createNotification(const NetworkPackage& np)
{

    const QString event = np.get<QString>("event");
    const QString phoneNumber = np.get<QString>("phoneNumber", i18n("unknown number"));

    QString content, type, icon;

    const QString title = device()->name();

    if (event == "ringing") {
        type = "callReceived";
        icon = "call-start";
        content = i18n("Incoming call from %1", phoneNumber);
    } else if (event == "missedCall") {
        type = "missedCall";
        icon = "call-start";
        content = i18n("Missed call from %1", phoneNumber);
    } else if (event == "sms") {
        type = "smsReceived";
        icon = "mail-receive";
        QString messageBody = np.get<QString>("messageBody","");
        content = i18n("SMS from %1: %2", phoneNumber, messageBody);
    } else if (event == "talking") {
        return NULL;
    } else {
        //TODO: return NULL if !debug
        type = "unknownEvent";
        icon = "pda";
        content = i18n("Unknown telephony event: %2", event);
    }

    kDebug(debugArea()) << "Creating notification with type:" << type;

    KNotification* notification = new KNotification(type, KNotification::CloseOnTimeout, this); //, KNotification::Persistent
    notification->setPixmap(KIcon(icon).pixmap(48, 48));
    notification->setComponentData(KComponentData("kdeconnect", "kdeconnect-kded"));
    notification->setTitle(title);
    notification->setText(content);

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
