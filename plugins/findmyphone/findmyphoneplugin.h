/**
 * SPDX-FileCopyrightText: 2014 Apoorv Parle <apparle@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_FINDMYPHONE_REQUEST QStringLiteral("kdeconnect.findmyphone.request")

class FindMyPhonePlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.findmyphone")

public:
    using KdeConnectPlugin::KdeConnectPlugin;

    Q_SCRIPTABLE void ring();

    QString dbusPath() const override;
};
