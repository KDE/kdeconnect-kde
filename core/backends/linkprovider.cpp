/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "linkprovider.h"
#include <QDBusConnection>
#include <core_debug.h>

LinkProvider::LinkProvider()
{
    // Terminate connections when we sleep or shut down.
    QDBusConnection::systemBus().connect(QStringLiteral("org.freedesktop.login1"),
                                         QStringLiteral("/org/freedesktop/login1"),
                                         QStringLiteral("org.freedesktop.login1.Manager"),
                                         QStringLiteral("PrepareForSleep"),
                                         this,
                                         SLOT(suspend(bool)));
    QDBusConnection::systemBus().connect(QStringLiteral("org.freedesktop.login1"),
                                         QStringLiteral("/org/freedesktop/login1"),
                                         QStringLiteral("org.freedesktop.login1.Manager"),
                                         QStringLiteral("PrepareForShutdown"),
                                         this,
                                         SLOT(suspend(bool)));
}

void LinkProvider::suspend(bool suspend)
{
    if (suspend) {
        qCDebug(KDECONNECT_CORE) << "Stopping connection for suspension";
        onStop();
    } else {
        qCDebug(KDECONNECT_CORE) << "Restarting connection after suspension";
        onStart();
    }
}

#include "moc_linkprovider.cpp"
