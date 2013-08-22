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

#include "notificationsdbusinterface.h"

#include <QDebug>
#include <QDBusConnection>

#include <KNotification>
#include <KIcon>

NotificationsDbusInterface::NotificationsDbusInterface(Device* device, QObject *parent)
    : QDBusAbstractAdaptor(parent)
    , mDevice(device)
    , mLastId(0)
{

}

NotificationsDbusInterface::~NotificationsDbusInterface()
{
    qDeleteAll(mNotifications);
}

QStringList NotificationsDbusInterface::activeNotifications()
{
    return mNotifications.keys();
}

void NotificationsDbusInterface::processPackage(const NetworkPackage& np)
{
    if (np.get<bool>("isCancel")) {
        removeNotification(np.get<QString>("id"));
    } else {
        Notification* noti = new Notification(np, this);
        addNotification(noti);

        if (!np.get<bool>("requestAnswer", false)) { //Do not show notifications for answers to a initial request
            KNotification* notification = new KNotification("notification");
            notification->setPixmap(KIcon("preferences-desktop-notification").pixmap(48, 48));
            notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
            notification->setTitle(mDevice->name());
            notification->setText(noti->appName() + ": " + noti->ticker());
            notification->sendEvent();
        }


    }
}

void NotificationsDbusInterface::addNotification(Notification* noti)
{
    const QString& internalId = noti->internalId();

    if (mInternalIdToPublicId.contains(internalId)) {
        removeNotification(internalId);
    }

    connect(noti, SIGNAL(dismissRequested(Notification*)),
            this, SLOT(dismissRequested(Notification*)));

    const QString& publicId = newId();
    mNotifications[publicId] = noti;
    mInternalIdToPublicId[internalId] = publicId;

    QDBusConnection::sessionBus().registerObject(mDevice->dbusPath()+"/notifications/"+publicId, noti, QDBusConnection::ExportScriptableContents);
    Q_EMIT notificationPosted(publicId);

}

void NotificationsDbusInterface::removeNotification(const QString& internalId)
{
    qDebug() << "removeNotification" << internalId;

    if (!mInternalIdToPublicId.contains(internalId)) {
        qDebug() << "Not found";
        return;
    }

    QString publicId = mInternalIdToPublicId[internalId];
    mInternalIdToPublicId.remove(internalId);

    if (!mNotifications.contains(publicId)) {
        qDebug() << "Not found";
        return;
    }

    Notification* noti = mNotifications[publicId];
    mNotifications.remove(publicId);

    //Deleting the notification will unregister it automatically
    //QDBusConnection::sessionBus().unregisterObject(mDevice->dbusPath()+"/notifications/"+publicId);
    noti->deleteLater();

    Q_EMIT notificationRemoved(publicId);
}

void NotificationsDbusInterface::dismissRequested(Notification* notification)
{
    const QString& internalId = notification->internalId();

    NetworkPackage np(PACKAGE_TYPE_NOTIFICATION);
    np.set<QString>("cancel", internalId);
    mDevice->sendPackage(np);

    //This should be called automatically back from server
    //removeNotification(internalId);
}

QString NotificationsDbusInterface::newId()
{
    return QString::number(++mLastId);
}

