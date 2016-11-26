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

#include "notificationsdbusinterface.h"
#include "notification_debug.h"

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_notifications.json", registerPlugin< NotificationsPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_NOTIFICATION, "kdeconnect.plugin.notification")

NotificationsPlugin::NotificationsPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    notificationsDbusInterface = new NotificationsDbusInterface(this);
}

void NotificationsPlugin::connected()
{
    NetworkPackage np(PACKAGE_TYPE_NOTIFICATION_REQUEST, {{"request", true}});
    sendPackage(np);
}

NotificationsPlugin::~NotificationsPlugin()
{
    qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Destroying NotificationsPlugin";
    //FIXME: Qt dbus does not allow to remove an adaptor! (it causes a crash in
    // the next dbus access to its parent). The implication of not deleting this
    // is that disabling the plugin leaks the interface. As a mitigation we are
    // cleaning up the notifications inside the adaptor here.
    //notificationsDbusInterface->deleteLater();
    notificationsDbusInterface->clearNotifications();
}

bool NotificationsPlugin::receivePackage(const NetworkPackage& np)
{
    if (np.get<bool>(QStringLiteral("request"))) return false;

    notificationsDbusInterface->processPackage(np);

    return true;
}

#include "notificationsplugin.moc"
