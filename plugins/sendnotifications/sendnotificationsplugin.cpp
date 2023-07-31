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
    notificationsListener = new NotificationsListener(this);
}

SendNotificationsPlugin::~SendNotificationsPlugin()
{
    delete notificationsListener;
}

#include "moc_sendnotificationsplugin.cpp"
#include "sendnotificationsplugin.moc"
