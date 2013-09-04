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

#include <QDebug>

#include <KNotification>
#include <KIcon>
#include <KLocalizedString>

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< PingPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_ping", "kdeconnect_ping") )

PingPlugin::PingPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    //qDebug() << "Ping plugin constructor for device" << device()->name();
}

PingPlugin::~PingPlugin()
{
    //qDebug() << "Ping plugin destructor for device" << device()->name();
}

bool PingPlugin::receivePackage(const NetworkPackage& np)
{

    if (np.type() != PACKAGE_TYPE_PING) return false;

    KNotification* notification = new KNotification("pingReceived"); //KNotification::Persistent
    notification->setPixmap(KIcon("dialog-ok").pixmap(48, 48));
    notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
    notification->setTitle(device()->name());
    notification->setText(np.get<QString>("message",i18n("Ping!")));
    notification->sendEvent();

    return true;

}
