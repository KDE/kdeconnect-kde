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

#include "notificationpackageinterface.h"

#include <QDebug>
#include <kicon.h>

KNotification* NotificationPackageInterface::createNotification(const QString& deviceName, const NetworkPackage& np) {

    QString npType = np.get<QString>("notificationType");

    QString title, content, type, icon;

    title = deviceName;

    if (npType == "ringing") {
        type = "callReceived";
        icon = "call-start";
        QString number =
        content = "Incoming call from " + np.get<QString>("phoneNumber","unknown number");
    } else if (npType == "missedCall") {
        type = "missedCall";
        icon = "call-start";
        content = "Missed call from " + np.get<QString>("phoneNumber","unknown number");
    } else if (npType == "sms") {
        type = "smsReceived";
        icon = "mail-receive";
        content = "SMS received from " + np.get<QString>("phoneNumber","unknown number");
    } else if (npType == "battery") {
        type = "battery100";
        icon = "battery-100";
        content = "Battery at " + np.get<QString>("batteryLevel") + "%";
    } else if (npType == "notification") {
        type = "pingReceived";
        icon = "dialog-ok";
        content = np.get<QString>("notificationContent");
    } else {
        //TODO: return NULL if !debug
        type = "unknownEvent";
        icon = "pda";
        content = "Unknown notification type: " + npType;
    }

    qDebug() << "Creating notification with type:" << type;

    KNotification* notification = new KNotification(type); //KNotification::Persistent
    notification->setPixmap(KIcon(icon).pixmap(48, 48));
    notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
    notification->setTitle(title);
    notification->setText(content);

    return notification;

}

bool NotificationPackageInterface::receivePackage(const Device& device, const NetworkPackage& np) {

    if (np.type() != PACKAGE_TYPE_NOTIFICATION) return false;

    if (np.get<bool>("isCancel")) {

        //It would be awesome to remove the old notification from the system tray here, but there is no way to do it :(
        //Now I realize why at the end of the day I have hundreds of notifications from facebook messages that I HAVE ALREADY READ,
        //...it's just because the telepathy client has no way to remove them! even when it knows that I have read those messages!

    } else {

        KNotification* n = createNotification(device.name(), np);
        if (n != NULL) n->sendEvent();

    }

    return true;

}
