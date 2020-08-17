/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef NOTIFICATIONSPLUGIN_H
#define NOTIFICATIONSPLUGIN_H

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_NOTIFICATION_REQUEST QStringLiteral("kdeconnect.notification.request")
#define PACKET_TYPE_NOTIFICATION_REPLY QStringLiteral("kdeconnect.notification.reply")
#define PACKET_TYPE_NOTIFICATION_ACTION QStringLiteral("kdeconnect.notification.action")


/*
 * This class is just a proxy for NotificationsDbusInterface
 * because it can not inherit from QDBusAbstractAdaptor and
 * KdeConnectPlugin at the same time (both are QObject)
 */
class NotificationsDbusInterface;

class NotificationsPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit NotificationsPlugin(QObject* parent, const QVariantList& args);
    ~NotificationsPlugin() override;

    bool receivePacket(const NetworkPacket& np) override;
    void connected() override;

protected:
    NotificationsDbusInterface* notificationsDbusInterface;
};

#endif
