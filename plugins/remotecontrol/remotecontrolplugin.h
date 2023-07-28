/**
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_MOUSEPAD_REQUEST QStringLiteral("kdeconnect.mousepad.request")

class RemoteControlPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.remotecontrol")

public:
    using KdeConnectPlugin::KdeConnectPlugin;

    QString dbusPath() const override;

    Q_SCRIPTABLE void moveCursor(const QPoint &p);
    Q_SCRIPTABLE void sendCommand(const QVariantMap &body);
};
