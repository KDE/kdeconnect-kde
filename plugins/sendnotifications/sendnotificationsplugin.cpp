/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sendnotificationsplugin.h"
#include "notifyingapplication.h"

#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
#include "dbusnotificationslistener.h"
#elif defined(Q_OS_WIN)
#include "windowsnotificationslistener.h"
#include <windows.h>

#include <appmodel.h> // GetCurrentPackageFullName
#endif
#include <KPluginFactory>

K_PLUGIN_CLASS_WITH_JSON(SendNotificationsPlugin, "kdeconnect_sendnotifications.json")

SendNotificationsPlugin::SendNotificationsPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    qRegisterMetaType<NotifyingApplication>("NotifyingApplication");
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN)
    notificationsListener = new DBusNotificationsListener(this);
#elif defined(Q_OS_WIN)
    std::uint32_t bufferLength = 100;
    std::array<wchar_t, 100> buffer;
    if (GetCurrentPackageFullName(&bufferLength, buffer.data()) == ERROR_SUCCESS) {
        notificationsListener = new WindowsNotificationsListener(this);
    }
#endif
}

SendNotificationsPlugin::~SendNotificationsPlugin()
{
    delete notificationsListener;
}

#include "moc_sendnotificationsplugin.cpp"
#include "sendnotificationsplugin.moc"
