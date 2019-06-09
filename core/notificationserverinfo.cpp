/**
 * Copyright 2019 Nicolas Fella <nicolas.fella@gmx.de>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "notificationserverinfo.h"

#include <QDBusMessage>
#include <QDBusPendingReply>
#include <QDBusPendingCallWatcher>

#include "dbushelper.h"

#include "core_debug.h"

NotificationServerInfo& NotificationServerInfo::instance()
{
    static NotificationServerInfo instance;
    return instance;
}

void NotificationServerInfo::init()
{
    QDBusMessage query = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.Notifications"), QStringLiteral("/org/freedesktop/Notifications"), QStringLiteral("org.freedesktop.Notifications"), QStringLiteral("GetCapabilities"));

    QDBusPendingReply<QStringList> reply = DbusHelper::sessionBus().asyncCall(query);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(reply, this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, reply, watcher] {
        watcher->deleteLater();

        if (reply.isError()) {
            qCWarning(KDECONNECT_CORE) << "Could not query capabilities from notifications server";
            return;
        }

        if (reply.value().contains(QLatin1String("x-kde-display-appname"))) {
            m_supportedHints |= X_KDE_DISPLAY_APPNAME;
        }

        if (reply.value().contains(QLatin1String("x-kde-origin-name"))) {
            m_supportedHints |= X_KDE_ORIGIN_NAME;
        }
    });
}

NotificationServerInfo::Hints NotificationServerInfo::supportedHints()
{
    return m_supportedHints;
}

