/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sendnotificationsplugin.h"

#include "notificationslistener.h"
#include "plugin_sendnotification_debug.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(SendNotificationsPlugin, "kdeconnect_sendnotifications.json")

SendNotificationsPlugin::SendNotificationsPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    // Disable the GVFS remote volume monitor (org.gtk.vfs.Daemon) since it's unused and can cause crashes
    if (!qEnvironmentVariableIsSet("GVFS_REMOTE_VOLUME_MONITOR_IGNORE")) {
        qputenv("GVFS_REMOTE_VOLUME_MONITOR_IGNORE", "1");
    }
    if (!qEnvironmentVariableIsSet("GIO_USE_VFS")) {
        qputenv("GIO_USE_VFS", "local");
    }
    notificationsListener = new NotificationsListener(this);
}

SendNotificationsPlugin::~SendNotificationsPlugin()
{
    delete notificationsListener;
}

bool SendNotificationsPlugin::receivePacket(const NetworkPacket &np)
{
    Q_UNUSED(np);
    return true;
}

void SendNotificationsPlugin::connected()
{
}

#include "sendnotificationsplugin.moc"
