/**
 * SPDX-FileCopyrightText: 2020 Aditya Mehra <aix.m@outlook.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "bigscreenplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDBusConnection>
#include <QDebug>
#include <QLoggingCategory>

#include <core/daemon.h>
#include <core/device.h>

K_PLUGIN_CLASS_WITH_JSON(BigscreenPlugin, "kdeconnect_bigscreen.json")

void BigscreenPlugin::receivePacket(const NetworkPacket &np)
{
    QString message = np.get<QString>(QStringLiteral("content"));
    /* Emit a signal that will be consumed by Plasma BigScreen:
     * https://invent.kde.org/plasma/plasma-bigscreen/-/blob/master/containments/homescreen/package/contents/ui/indicators/KdeConnect.qml
     */
    Q_EMIT messageReceived(message);
}

QString BigscreenPlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/bigscreen").arg(device()->id());
}

#include "bigscreenplugin.moc"
#include "moc_bigscreenplugin.cpp"
