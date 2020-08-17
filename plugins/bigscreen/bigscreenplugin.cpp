/**
 * SPDX-FileCopyrightText: 2020 Aditya Mehra <aix.m@outlook.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bigscreenplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QLoggingCategory>

#include <core/device.h>
#include <core/daemon.h>

K_PLUGIN_CLASS_WITH_JSON(BigscreenPlugin, "kdeconnect_bigscreen.json")

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_BIGSCREEN, "kdeconnect.plugin.bigscreen")

BigscreenPlugin::BigscreenPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

BigscreenPlugin::~BigscreenPlugin() = default;

bool BigscreenPlugin::receivePacket(const NetworkPacket& np)
{
    QString message = np.get<QString>(QStringLiteral("content"));
    Q_EMIT messageReceived(message);
    return true;
}

QString BigscreenPlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/bigscreen");
}

#include "bigscreenplugin.moc"
