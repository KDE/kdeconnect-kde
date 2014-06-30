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

#include "pingplugin.h"

#include <KNotification>
#include <KIcon>
#include <KLocalizedString>

#include <core/kdebugnamespace.h>
#include <core/device.h>
#include <QDBusConnection>

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< PingPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_ping", "kdeconnect-plugins") )

PingPlugin::PingPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    //kDebug(debugArea()) << "Ping plugin constructor for device" << device()->name();
}

PingPlugin::~PingPlugin()
{
    //kDebug(debugArea()) << "Ping plugin destructor for device" << device()->name();
}

bool PingPlugin::receivePackage(const NetworkPackage& np)
{
    KNotification* notification = new KNotification("pingReceived"); //KNotification::Persistent
    notification->setPixmap(KIcon("dialog-ok").pixmap(48, 48));
    notification->setComponentData(KComponentData("kdeconnect", "kdeconnect-kded"));
    notification->setTitle(device()->name());
    notification->setText(np.get<QString>("message",i18n("Ping!"))); //This can be a source of spam
    notification->sendEvent();

    return true;

}

void PingPlugin::sendPing()
{
    NetworkPackage np(PACKAGE_TYPE_PING);
    bool success = sendPackage(np);
    kDebug(debugArea()) << "sendPing:" << success;
}

void PingPlugin::connected()
{
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportAllContents);
}

QString PingPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/ping";
}
