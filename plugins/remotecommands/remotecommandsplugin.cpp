/**
 * Copyright 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
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

#include "remotecommandsplugin.h"

#include <KPluginFactory>

#include <QDBusConnection>
#include <QProcess>
#include <QDir>
#include <QLoggingCategory>
#include <QSettings>
#include <QJsonDocument>
#include <KShell>

#include <core/networkpackage.h>
#include <core/device.h>

#define PACKAGE_TYPE_RUNCOMMAND_REQUEST QLatin1String("kdeconnect.runcommand.request")

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_remotecommands.json", registerPlugin< RemoteCommandsPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_REMOTECOMMANDS, "kdeconnect.plugin.remotecommands")

RemoteCommandsPlugin::RemoteCommandsPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , m_commands("{}")
{
}

RemoteCommandsPlugin::~RemoteCommandsPlugin() = default;

bool RemoteCommandsPlugin::receivePackage(const NetworkPackage& np)
{
    if (np.has(QStringLiteral("commandList"))) {
        setCommands(np.get<QByteArray>(QStringLiteral("commandList")));
        return true;
    }

    return false;
}

void RemoteCommandsPlugin::connected()
{
    NetworkPackage np(PACKAGE_TYPE_RUNCOMMAND_REQUEST, {{"requestCommandList", true}});
    sendPackage(np);
}

QString RemoteCommandsPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/remotecommands";
}

void RemoteCommandsPlugin::setCommands(const QByteArray& cmds)
{
    if (m_commands != cmds) {
        m_commands = cmds;
        Q_EMIT commandsChanged(m_commands);
    }
}

void RemoteCommandsPlugin::triggerCommand(const QString& key)
{
    NetworkPackage np(PACKAGE_TYPE_RUNCOMMAND_REQUEST, {{ "key", key }});
    sendPackage(np);
}

#include "remotecommandsplugin.moc"
