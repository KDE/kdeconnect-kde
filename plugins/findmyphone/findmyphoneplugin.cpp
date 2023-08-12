/**
 * SPDX-FileCopyrightText: 2014 Apoorv Parle <apparle@gmail.com>
 * SPDX-FileCopyrightText: 2015 David Edmundson <davidedmundson@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "findmyphoneplugin.h"

#include <QDBusConnection>
#include <QDebug>

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(FindMyPhonePlugin, "kdeconnect_findmyphone.json")

void FindMyPhonePlugin::ring()
{
    NetworkPacket np(PACKET_TYPE_FINDMYPHONE_REQUEST);
    sendPacket(np);
}

QString FindMyPhonePlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/findmyphone").arg(device()->id());
}

#include "findmyphoneplugin.moc"
#include "moc_findmyphoneplugin.cpp"
