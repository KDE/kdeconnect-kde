/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "notificationserverinfo.h"

#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

#include "dbushelper.h"

#include "core_debug.h"

NotificationServerInfo &NotificationServerInfo::instance()
{
    static NotificationServerInfo instance;
    return instance;
}

void NotificationServerInfo::init()
{
    QDBusMessage query = QDBusMessage::createMethodCall(QStringLiteral("org.freedesktop.Notifications"),
                                                        QStringLiteral("/org/freedesktop/Notifications"),
                                                        QStringLiteral("org.freedesktop.Notifications"),
                                                        QStringLiteral("GetCapabilities"));

    QDBusPendingReply<QStringList> reply = QDBusConnection::sessionBus().asyncCall(query);
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

#include "moc_notificationserverinfo.cpp"
