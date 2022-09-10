/**
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "remotecommandsplugin.h"

#include <KPluginFactory>

#include <core/device.h>
#include <core/networkpacket.h>

#include "plugin_remotecommands_debug.h"

#define PACKET_TYPE_RUNCOMMAND_REQUEST QLatin1String("kdeconnect.runcommand.request")

K_PLUGIN_CLASS_WITH_JSON(RemoteCommandsPlugin, "kdeconnect_remotecommands.json")

RemoteCommandsPlugin::RemoteCommandsPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_commands("{}")
    , m_canAddCommand(false)
{
}

RemoteCommandsPlugin::~RemoteCommandsPlugin() = default;

bool RemoteCommandsPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.has(QStringLiteral("commandList"))) {
        m_canAddCommand = np.get<bool>(QStringLiteral("canAddCommand"));
        setCommands(np.get<QByteArray>(QStringLiteral("commandList")));
        return true;
    }

    return false;
}

void RemoteCommandsPlugin::connected()
{
    NetworkPacket np(PACKET_TYPE_RUNCOMMAND_REQUEST, {{QStringLiteral("requestCommandList"), true}});
    sendPacket(np);
}

QString RemoteCommandsPlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/remotecommands");
}

void RemoteCommandsPlugin::setCommands(const QByteArray &cmds)
{
    if (m_commands != cmds) {
        m_commands = cmds;
        Q_EMIT commandsChanged(m_commands);
    }
}

void RemoteCommandsPlugin::triggerCommand(const QString &key)
{
    NetworkPacket np(PACKET_TYPE_RUNCOMMAND_REQUEST, {{QStringLiteral("key"), key}});
    sendPacket(np);
}

void RemoteCommandsPlugin::editCommands()
{
    NetworkPacket np(PACKET_TYPE_RUNCOMMAND_REQUEST, {{QStringLiteral("setup"), true}});
    sendPacket(np);
}

#include "remotecommandsplugin.moc"
