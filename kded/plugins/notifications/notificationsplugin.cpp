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

#include <KIcon>

#include "../../kdebugnamespace.h"
#include "notificationsdbusinterface.h"

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< NotificationsPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_notifications", "kdeconnect-kded") )

NotificationsPlugin::NotificationsPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    notificationsDbusInterface = new NotificationsDbusInterface(device(), parent);
}

void NotificationsPlugin::connected()
{
    NetworkPackage np(PACKAGE_TYPE_NOTIFICATION);
    np.set("request", true);
    device()->sendPackage(np);
}

NotificationsPlugin::~NotificationsPlugin()
{
    //FIXME: Qt dbus does not allow to remove an adaptor! (it causes a crash in
    // the next dbus access to its parent). The implication of not deleting this
    // is that disabling the plugin does not remove the interface (that will
    // return outdated values) and that enabling it again instantiates a second
    // adaptor.

    //notificationsDbusInterface->deleteLater();

}

bool NotificationsPlugin::receivePackage(const NetworkPackage& np)
{
    if (np.get<bool>("request")) return false;

    notificationsDbusInterface->processPackage(np);

    return true;
}

   
