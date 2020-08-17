/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "notificationsdbusinterface.h"
#include "plugin_notification_debug.h"
#include "notification.h"

#include <core/device.h>
#include <core/kdeconnectplugin.h>
#include <dbushelper.h>

#include "notificationsplugin.h"
#include "sendreplydialog.h"

//In older Qt released, qAsConst isnt available
#include "qtcompat_p.h"

NotificationsDbusInterface::NotificationsDbusInterface(KdeConnectPlugin* plugin)
    : QDBusAbstractAdaptor(const_cast<Device*>(plugin->device()))
    , m_device(plugin->device())
    , m_plugin(plugin)
    , m_lastId(0)
{

}

NotificationsDbusInterface::~NotificationsDbusInterface()
{
    qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Destroying NotificationsDbusInterface";
}

void NotificationsDbusInterface::clearNotifications()
{
    qDeleteAll(m_notifications);
    m_notifications.clear();
    Q_EMIT allNotificationsRemoved();
}

QStringList NotificationsDbusInterface::activeNotifications()
{
    return m_notifications.keys();
}

void NotificationsDbusInterface::notificationReady()
{
    Notification* noti = static_cast<Notification*>(sender());
    disconnect(noti, &Notification::ready, this, &NotificationsDbusInterface::notificationReady);
    addNotification(noti);
}

void NotificationsDbusInterface::processPacket(const NetworkPacket& np)
{
    if (np.get<bool>(QStringLiteral("isCancel"))) {
        QString id = np.get<QString>(QStringLiteral("id"));
        // cut off kdeconnect-android's prefix if there:
        if (id.startsWith(QLatin1String("org.kde.kdeconnect_tp::")))
            id = id.mid(id.indexOf(QLatin1String("::")) + 2);
        removeNotification(id);
        return;
    }

    QString id = np.get<QString>(QStringLiteral("id"));

    Notification* noti = nullptr;

    if (!m_internalIdToPublicId.contains(id)) {
        noti = new Notification(np, m_plugin->device(), this);

        if (noti->isReady()) {
            addNotification(noti);
        } else {
            connect(noti, &Notification::ready, this, &NotificationsDbusInterface::notificationReady);
        }
    } else {
        QString pubId = m_internalIdToPublicId.value(id);
        noti = m_notifications.value(pubId);
        noti->update(np);
    }
}

void NotificationsDbusInterface::addNotification(Notification* noti)
{
    const QString& internalId = noti->internalId();

    if (m_internalIdToPublicId.contains(internalId)) {
        removeNotification(internalId);
    }

    //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "addNotification" << internalId;

    connect(noti, &Notification::dismissRequested,
            this, &NotificationsDbusInterface::dismissRequested);

    connect(noti, &Notification::replyRequested, this, [this,noti]{
        replyRequested(noti);
    });

    connect(noti, &Notification::actionTriggered, this, &NotificationsDbusInterface::sendAction);

    const QString& publicId = newId();
    m_notifications[publicId] = noti;
    m_internalIdToPublicId[internalId] = publicId;

    DBusHelper::sessionBus().registerObject(m_device->dbusPath() + QStringLiteral("/notifications/") + publicId, noti, QDBusConnection::ExportScriptableContents);
    Q_EMIT notificationPosted(publicId);
}

void NotificationsDbusInterface::removeNotification(const QString& internalId)
{
    //qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "removeNotification" << internalId;

    if (!m_internalIdToPublicId.contains(internalId)) {
        qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Not found noti by internal Id: " << internalId;
        return;
    }

    QString publicId = m_internalIdToPublicId.take(internalId);

    Notification* noti = m_notifications.take(publicId);
    if (!noti) {
        qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Not found noti by public Id: " << publicId;
        return;
    }

    //Deleting the notification will unregister it automatically
    //DBusHelper::sessionBus().unregisterObject(mDevice->dbusPath()+"/notifications/"+publicId);
    noti->deleteLater();

    Q_EMIT notificationRemoved(publicId);
}

void NotificationsDbusInterface::dismissRequested(const QString& internalId)
{
    NetworkPacket np(PACKET_TYPE_NOTIFICATION_REQUEST);
    np.set<QString>(QStringLiteral("cancel"), internalId);
    m_plugin->sendPacket(np);

    //Workaround: we erase notifications without waiting a response from the
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
    dialog->raise();
}

void NotificationsDbusInterface::sendReply(const QString& replyId, const QString& message)
{
    NetworkPacket np(PACKET_TYPE_NOTIFICATION_REPLY);
    np.set<QString>(QStringLiteral("requestReplyId"), replyId);
    np.set<QString>(QStringLiteral("message"), message);
    m_plugin->sendPacket(np);
}

void NotificationsDbusInterface::sendAction(const QString& key, const QString& action)
{
    NetworkPacket np(PACKET_TYPE_NOTIFICATION_ACTION);
    np.set<QString>(QStringLiteral("key"), key);
    np.set<QString>(QStringLiteral("action"), action);
    m_plugin->sendPacket(np);
}

QString NotificationsDbusInterface::newId()
{
    return QString::number(++m_lastId);
}
