/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "core/kdeconnectplugin.h"

/*
 * This class is just a proxy for NotificationsDbusInterface
 * because it can not inherit from QDBusAbstractAdaptor and
 * KdeConnectPlugin at the same time (both are QObject)
 */
class NotificationsDbusInterface;
class NotificationsListener;

class SendNotificationsPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit SendNotificationsPlugin(QObject *parent, const QVariantList &args);
    ~SendNotificationsPlugin() override;

protected:
    NotificationsListener *notificationsListener;
};
