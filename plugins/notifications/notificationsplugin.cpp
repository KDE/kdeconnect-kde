/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "notificationsplugin.h"

#include "notificationsdbusinterface.h"
#include "plugin_notification_debug.h"

#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(NotificationsPlugin, "kdeconnect_notifications.json")

NotificationsPlugin::NotificationsPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    notificationsDbusInterface = new NotificationsDbusInterface(this);
}

void NotificationsPlugin::connected()
{
    NetworkPacket np(PACKET_TYPE_NOTIFICATION_REQUEST, {{QStringLiteral("request"), true}});
    sendPacket(np);
}

NotificationsPlugin::~NotificationsPlugin()
{
    qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Destroying NotificationsPlugin";
    //FIXME: Qt dbus does not allow to remove an adaptor! (it causes a crash in
    // the next dbus access to its parent). The implication of not deleting this
    // is that disabling the plugin leaks the interface. As a mitigation we are
    // cleaning up the notifications inside the adaptor here.
    //notificationsDbusInterface->deleteLater();
    notificationsDbusInterface->clearNotifications();
}

bool NotificationsPlugin::receivePacket(const NetworkPacket& np)
{
    if (np.get<bool>(QStringLiteral("request"))) return false;

    notificationsDbusInterface->processPacket(np);

    return true;
}

#include "notificationsplugin.moc"
