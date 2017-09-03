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

#ifndef NOTIFICATIONSPLUGIN_H
#define NOTIFICATIONSPLUGIN_H

#include <knotification.h>

#include <core/kdeconnectplugin.h>

#define PACKAGE_TYPE_NOTIFICATION_REQUEST QStringLiteral("kdeconnect.notification.request")
#define PACKAGE_TYPE_NOTIFICATION_REPLY QStringLiteral("kdeconnect.notification.reply")

/*
 * This class is just a proxy for NotificationsDbusInterface
 * because it can not inherit from QDBusAbstractAdaptor and
 * KdeConnectPlugin at the same time (both are QObject)
 */
class NotificationsDbusInterface;
class NotificationsListener;

class NotificationsPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit NotificationsPlugin(QObject* parent, const QVariantList& args);
    ~NotificationsPlugin() override;
    
    bool receivePackage(const NetworkPackage& np) override;
    void connected() override;

protected:
    NotificationsDbusInterface* notificationsDbusInterface;
    NotificationsListener* notificationsListener;

};

#endif
