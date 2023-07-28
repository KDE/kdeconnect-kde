/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_PING QStringLiteral("kdeconnect.ping")

class PingPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.ping")

public:
    using KdeConnectPlugin::KdeConnectPlugin;

    Q_SCRIPTABLE void sendPing();
    Q_SCRIPTABLE void sendPing(const QString &customMessage);

    void receivePacket(const NetworkPacket &np) override;
    QString dbusPath() const override;
};
