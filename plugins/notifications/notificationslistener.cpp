/**
 * Copyright 2015 Holger Kaelberer <holger.k@elberer.de>
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

#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDebug>
#include <QLoggingCategory>

#include <core/device.h>
#include <core/kdeconnectplugin.h>

#include "notificationslistener.h"
#include "notificationsplugin.h"
#include "notification_debug.h"
#include "notificationsdbusinterface.h"

NotificationsListener::NotificationsListener(KdeConnectPlugin* aPlugin,
                                             NotificationsDbusInterface* aDbusInterface)
    : QDBusAbstractAdaptor(aPlugin),
      mPlugin(aPlugin),
      dbusInterface(aDbusInterface)
{
    bool ret = QDBusConnection::sessionBus()
                .registerObject("/org/freedesktop/Notifications",
                                this,
                                QDBusConnection::ExportScriptableContents);
    if (!ret)
        qCWarning(KDECONNECT_PLUGIN_NOTIFICATION)
                << "Error registering notifications listener for device"
                << mPlugin->device()->name() << ":"
                << QDBusConnection::sessionBus().lastError();
    else
        qCDebug(KDECONNECT_PLUGIN_NOTIFICATION)
                << "Registered notifications listener for device"
                << mPlugin->device()->name();

    QDBusInterface iface("org.freedesktop.DBus", "/org/freedesktop/DBus",
                         "org.freedesktop.DBus");
    iface.call("AddMatch",
               "interface='org.freedesktop.Notifications',member='Notify',type='method_call',eavesdrop='true'");
}

NotificationsListener::~NotificationsListener()
{
    qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Destroying NotificationsListener";
    QDBusInterface iface("org.freedesktop.DBus", "/org/freedesktop/DBus",
                         "org.freedesktop.DBus");
    QDBusMessage res = iface.call("RemoveMatch",
                                  "interface='org.freedesktop.Notifications',member='Notify',type='method_call',eavesdrop='true'");
    QDBusConnection::sessionBus().unregisterObject("/org/freedesktop/Notifications");
}

uint NotificationsListener::Notify(const QString &appName, uint replacesId,
                                   const QString &appIcon,
                                   const QString &summary, const QString &body,
                                   const QStringList &actions,
                                   const QVariantMap &hints, int timeout)
{
    static int id = 0;
    qCDebug(KDECONNECT_PLUGIN_NOTIFICATION) << "Got notification appName=" << appName << "replacesId=" << replacesId << "appIcon=" << appIcon << "summary=" << summary << "body=" << body << "actions=" << actions << "hints=" << hints << "timeout=" << timeout;
    Q_UNUSED(hints);
    Q_UNUSED(actions);
    Q_UNUSED(appIcon);
    Q_UNUSED(body);

    // skip our own notifications
    if (appName == QLatin1String("KDE Connect"))
        return 0;

    NetworkPackage np(PACKAGE_TYPE_NOTIFICATION);
    np.set("id", QString::number(replacesId > 0 ? replacesId : ++id));
    np.set("appName", appName);
    np.set("ticker", summary);
    np.set("isClearable", timeout == 0);  // KNotifications are persistent if
                                          // timeout == 0, for other notifications
                                          // clearability is pointless

    Notification *notification = new Notification(np, QString(), this);
    dbusInterface->addNotification(notification);

    mPlugin->sendPackage(np);

    return (replacesId > 0 ? replacesId : id);
}
