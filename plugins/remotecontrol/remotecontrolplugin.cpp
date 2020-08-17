/**
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "remotecontrolplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QPoint>

#include <core/device.h>
#include "plugin_remotecontrol_debug.h"

K_PLUGIN_CLASS_WITH_JSON(RemoteControlPlugin, "kdeconnect_remotecontrol.json")

RemoteControlPlugin::RemoteControlPlugin(QObject* parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
}

RemoteControlPlugin::~RemoteControlPlugin()
{}

void RemoteControlPlugin::moveCursor(const QPoint &p)
{
    NetworkPacket np(PACKET_TYPE_MOUSEPAD_REQUEST, {
        {QStringLiteral("dx"), p.x()},
        {QStringLiteral("dy"), p.y()}
    });
    sendPacket(np);
}

void RemoteControlPlugin::sendCommand(const QString &name, bool val)
{
    NetworkPacket np(PACKET_TYPE_MOUSEPAD_REQUEST, {{name, val}});
    sendPacket(np);
}

QString RemoteControlPlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/remotecontrol");
}

#include "remotecontrolplugin.moc"
