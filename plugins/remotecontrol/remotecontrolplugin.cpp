/**
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#include "remotecontrolplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QPoint>
#include <QLoggingCategory>

#include <core/device.h>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_remotecontrol.json", registerPlugin< RemoteControlPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_REMOTECONTROL, "kdeconnect.plugin.remotecontrol")

RemoteControlPlugin::RemoteControlPlugin(QObject* parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
}

RemoteControlPlugin::~RemoteControlPlugin()
{}

void RemoteControlPlugin::moveCursor(const QPoint &p)
{
    NetworkPackage np(PACKAGE_TYPE_MOUSEPAD_REQUEST, {
        {"dx", p.x()},
        {"dy", p.y()}
    });
    sendPackage(np);
}

void RemoteControlPlugin::sendCommand(const QString &name, bool val)
{
    NetworkPackage np(PACKAGE_TYPE_MOUSEPAD_REQUEST, {{name, val}});
    sendPackage(np);
}

QString RemoteControlPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/remotecontrol";
}

#include "remotecontrolplugin.moc"
