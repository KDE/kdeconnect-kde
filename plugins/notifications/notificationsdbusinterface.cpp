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
#include "notification_debug.h"
#include "notification.h"

#include <QDBusConnection>

#include <core/device.h>
#include <core/kdeconnectplugin.h>

#include "notificationsplugin.h"
#include "sendreplydialog.h"

NotificationsDbusInterface::NotificationsDbusInterface(KdeConnectPlugin* plugin)
    : QDBusAbstractAdaptor(const_cast<Device*>(plugin->device()))
    , mDevice(plugin->device())
    , mPlugin(plugin)
    , mLastId(0)
{

}

NotificationsDbusInterface::~NotificationsDbusInterface()
{
    qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Destroying NotificationsDbusInterface";
}

void NotificationsDbusInterface::clearNotifications()
{
    qDeleteAll(mNotifications);
    mNotifications.clear();
    Q_EMIT allNotificationsRemoved();
}

QStringList NotificationsDbusInterface::activeNotifications()
{
    return mNotifications.keys();
}

void NotificationsDbusInterface::processPackage(const NetworkPackage& np)
{
    if (np.get<bool>(QStringLiteral("isCancel"))) {
        QString id = np.get<QString>(QStringLiteral("id"));
        // cut off kdeconnect-android's prefix if there:
        if (id.startsWith(QLatin1String("org.kde.kdeconnect_tp::")))
            id = id.mid(id.indexOf(QLatin1String("::")) + 2);
        removeNotification(id);
    } else if (np.get<bool>(QStringLiteral("isRequest"))) {
        Q_FOREACH (const auto& n, mNotifications) {
            NetworkPackage np(PACKAGE_TYPE_NOTIFICATION_REQUEST, {
                {"id", n->internalId()},
                {"appName", n->appName()},
                {"ticker", n->ticker()},
                {"isClearable", n->dismissable()},
                {"requestAnswer", true}
            });
            mPlugin->sendPackage(np);
        }
    } else if(np.get<bool>(QStringLiteral("requestAnswer"), false)) {

    } else {
        QString id = np.get<QString>(QStringLiteral("id"));

        if (!mInternalIdToPublicId.contains(id)) {
            Notification* noti = new Notification(np, this);
            addNotification(noti);
        } else {
            QString pubId = mInternalIdToPublicId[id];
            mNotifications[pubId]->update(np);
        }
    }
}

void NotificationsDbusInterface::addNotification(Notification* noti)
{
    const QString& internalId = noti->internalId();

    if (mInternalIdToPublicId.contains(internalId)) {
        removeNotification(internalId);
    }

    //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "addNotification" << internalId;

    connect(noti, &Notification::dismissRequested,
            this, &NotificationsDbusInterface::dismissRequested);
    
    connect(noti, &Notification::replyRequested, this, [this,noti]{ 
        replyRequested(noti); 
    });

    const QString& publicId = newId();
    mNotifications[publicId] = noti;
    mInternalIdToPublicId[internalId] = publicId;

    QDBusConnection::sessionBus().registerObject(mDevice->dbusPath()+"/notifications/"+publicId, noti, QDBusConnection::ExportScriptableContents);
    Q_EMIT notificationPosted(publicId);
}

void NotificationsDbusInterface::removeNotification(const QString& internalId)
{
    //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "removeNotification" << internalId;

    if (!mInternalIdToPublicId.contains(internalId)) {
        qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Not found noti by internal Id: " << internalId;
        return;
    }

    QString publicId = mInternalIdToPublicId.take(internalId);

    Notification* noti = mNotifications.take(publicId);
    if (!noti) {
        qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Not found noti by public Id: " << publicId;
        return;
    }

    //Deleting the notification will unregister it automatically
    //QDBusConnection::sessionBus().unregisterObject(mDevice->dbusPath()+"/notifications/"+publicId);
    noti->deleteLater();

    Q_EMIT notificationRemoved(publicId);
}

void NotificationsDbusInterface::dismissRequested(const QString& internalId)
{
    NetworkPackage np(PACKAGE_TYPE_NOTIFICATION_REQUEST);
    np.set<QString>(QStringLiteral("cancel"), internalId);
    mPlugin->sendPackage(np);

    //Workaround: we erase notifications without waiting a repsonse from the
    //phone because we won't receive a response if we are out of sync and this
    //notification no longer exists. Ideally, each time we reach the phone
    //after some time disconnected we should re-sync all the notifications.
    removeNotification(internalId);
}

void NotificationsDbusInterface::replyRequested(Notification* noti)
{
    QString replyId = noti->replyId();
    QString appName = noti->appName();
    QString originalMessage = noti->ticker();
    SendReplyDialog* dialog = new SendReplyDialog(originalMessage, replyId, appName);
    connect(dialog, &SendReplyDialog::sendReply, this, &NotificationsDbusInterface::sendReply);
    dialog->show();
}

void NotificationsDbusInterface::sendReply(const QString& replyId, const QString& message)
{
    NetworkPackage np(PACKAGE_TYPE_NOTIFICATION_REPLY);
    np.set<QString>(QStringLiteral("requestReplyId"), replyId);
    np.set<QString>(QStringLiteral("message"), message);
    mPlugin->sendPackage(np);
}

QString NotificationsDbusInterface::newId()
{
    return QString::number(++mLastId);
}
