/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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

    void processPacket(const NetworkPacket& np);
    void clearNotifications();
    void dismissRequested(const QString& notification);
    void replyRequested(Notification* noti);
    void addNotification(Notification* noti);

public Q_SLOTS:
    Q_SCRIPTABLE QStringList activeNotifications();
    Q_SCRIPTABLE void sendReply(const QString& replyId, const QString& message);
    Q_SCRIPTABLE void sendAction(const QString& key, const QString& action);

Q_SIGNALS:
    Q_SCRIPTABLE void notificationPosted(const QString& publicId);
    Q_SCRIPTABLE void notificationRemoved(const QString& publicId);
    Q_SCRIPTABLE void notificationUpdated(const QString& publicId);
    Q_SCRIPTABLE void allNotificationsRemoved();

private /*methods*/:
    void removeNotification(const QString& internalId);
    QString newId(); //Generates successive identifiers to use as public ids
    void notificationReady();

private /*attributes*/:
    const Device* m_device;
    KdeConnectPlugin* m_plugin;
    QHash<QString, QPointer<Notification>> m_notifications;
    QHash<QString, QString> m_internalIdToPublicId;
    int m_lastId;
};

#endif
