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

#include "notificationpackagereceiver.h"

#include <kicon.h>

KNotification* NotificationPackageReceiver::createNotification(const NetworkPackage& np) {

    QString title, type, icon;

    if (np.type() == "RINGING") {
        title = "Incoming call";
        type = "callReceived";
        icon = "call-start";
    } else if (np.type() == "MISSED") {
        title = "Missed call";
        type = "callMissed";
        icon = "call-start";
    } else if (np.type() == "SMS") {
        title = "SMS Received";
        type = "smsReceived";
        icon = "mail-receive";
    } else if (np.type() == "BATTERY") {
        title = "Battery status";
        type = "battery100";
        icon = "battery-100"; // Here we need to take all different cases into account. All
                                // possible steps on battery charge level and state (discharging
                                // or charging)
    } else if (np.type() == "NOTIFY") {
        title = "Notification";
        type = "pingReceived";
        icon = "dialog-ok";
    } else if (np.type() == "PING") {
        title = "Ping!";
        type = "pingReceived";
        icon = "dialog-ok";
    } else {
        //TODO: return if !debug
        title = "Unknown";
        type = "unknownEvent";
        icon = "pda";
    }


    KNotification* notification = new KNotification(type); //KNotification::Persistent
    notification->setPixmap(KIcon(icon).pixmap(48, 48));
    notification->setComponentData(KComponentData("androidshine", "androidshine"));
    notification->setTitle(title);
    notification->setText(np.body());

    return notification;

}

bool NotificationPackageReceiver::receivePackage(const NetworkPackage& np) {

    if (np.isCancel()) {

        //system("qdbus org.mpris.MediaPlayer2.spotify /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.PlayPause");

        //It would be awesome to remove the old notification from the system tray here, but there is no way to do it :(
        //Now I realize why at the end of the day I have hundreds of notifications from facebook messages that I HAVE ALREADY READ,
        //...it's just because the telepathy client has no way to remove them! even when it knows that I have read those messages lol
        //Here we have our awesome KDE notifications system, unusable and meant to fuck the user.

    } else {

        KNotification* n = createNotification(np);
        n->sendEvent();

        //system("qdbus org.mpris.MediaPlayer2.spotify /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.Pause");


    }

    return true;


}
