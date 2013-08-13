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

#include "notificationsplugin.h"

#include <QDebug>
#include <kicon.h>


K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< NotificationPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_notifications", "kdeconnect_notifications") )


NotificationsPlugin::NotificationsPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    //TODO: Split in EventNotificationInterface and NotificationDrawerSyncInterface
    //TODO: Add low battery notifications
    
    trayIcon = new KStatusNotifierItem(parent);
    trayIcon->setIconByName("pda");
    trayIcon->setTitle("KdeConnect");
    connect(trayIcon,SIGNAL(activateRequested(bool,QPoint)),this,SLOT(showPendingNotifications()));
}

KNotification* NotificationsPlugin::createNotification(const QString& deviceName, const NetworkPackage& np)
{

    QString id = QString::number(np.id());

    QString npType = np.get<QString>("notificationType");

    QString title, content, type, icon;
    bool transient;

    title = deviceName;

    if (npType == "ringing") {
        type = "callReceived";
        icon = "call-start";
        content = "Incoming call from " + np.get<QString>("phoneNumber","unknown number");
        transient = false;
    } else if (npType == "missedCall") {
        type = "missedCall";
        icon = "call-start";
        content = "Missed call from " + np.get<QString>("phoneNumber","unknown number");
        transient = true;
    } else if (npType == "sms") {
        type = "smsReceived";
        icon = "mail-receive";
        content = "SMS from "
            + np.get<QString>("phoneNumber","unknown number")
            + ":\n"
            + np.get<QString>("messageBody","");
            transient = true;
    } else {
        //TODO: return NULL if !debug
        type = "unknownEvent";
        icon = "pda";
        content = "Unknown notification type: " + npType;
        transient = false;
    }

    qDebug() << "Creating notification with type:" << type;


    if (transient) {
        trayIcon->setStatus(KStatusNotifierItem::Active);

        KNotification* notification = new KNotification(type); //KNotification::Persistent
        notification->setPixmap(KIcon(icon).pixmap(48, 48));
        notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
        notification->setTitle(title);
        notification->setText(content);

        pendingNotifications.insert(id, notification);
    }


    KNotification* notification = new KNotification(type); //KNotification::Persistent
    notification->setPixmap(KIcon(icon).pixmap(48, 48));
    notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
    notification->setTitle(title);
    notification->setText(content);
    notification->setProperty("id",id);

    connect(notification,SIGNAL(activated()),this,SLOT(notificationAttended()));
    connect(notification,SIGNAL(closed()),this,SLOT(notificationAttended()));

    return notification;

}

void NotificationsPlugin::notificationAttended()
{
    KNotification* normalNotification = (KNotification*)sender();
    QString id = normalNotification->property("id").toString();
    if (pendingNotifications.contains(id)) {
        delete pendingNotifications[id];
        pendingNotifications.remove(id);
        if (pendingNotifications.isEmpty()) {
            trayIcon->setStatus(KStatusNotifierItem::Passive);
        }
    }
}

void NotificationsPlugin::showPendingNotifications()
{
    trayIcon->setStatus(KStatusNotifierItem::Passive);
    Q_FOREACH (KNotification* notification, pendingNotifications) {
        notification->sendEvent();
    }
    pendingNotifications.clear();
}

bool NotificationsPlugin::receivePackage(const Device& device, const NetworkPackage& np)
{

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
