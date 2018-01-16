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

#ifndef NOTIFICATIONSDBUSINTERFACE_H
#define NOTIFICATIONSDBUSINTERFACE_H

#include <QDBusAbstractAdaptor>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QPointer>

#include "notification.h"

class KdeConnectPlugin;
class Device;

class NotificationsDbusInterface
    : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.notifications")

public:
    explicit NotificationsDbusInterface(KdeConnectPlugin* plugin);
    ~NotificationsDbusInterface() override;

    void processPackage(const NetworkPackage& np);
    void clearNotifications();
    void dismissRequested(const QString& notification);
    void replyRequested(Notification* noti);
    void addNotification(Notification* noti);

public Q_SLOTS:
    Q_SCRIPTABLE QStringList activeNotifications();
    Q_SCRIPTABLE void sendReply(const QString& replyId, const QString& message);

Q_SIGNALS:
    Q_SCRIPTABLE void notificationPosted(const QString& publicId);
    Q_SCRIPTABLE void notificationRemoved(const QString& publicId);
    Q_SCRIPTABLE void notificationUpdated(const QString& publicId);
    Q_SCRIPTABLE void allNotificationsRemoved();

private /*methods*/:
    void removeNotification(const QString& internalId);
    QString newId(); //Generates successive identifitiers to use as public ids

private /*attributes*/:
    const Device* m_device;
    KdeConnectPlugin* m_plugin;
    QHash<QString, QPointer<Notification>> m_notifications;
    QHash<QString, QString> m_internalIdToPublicId;
    int m_lastId;
};

#endif
