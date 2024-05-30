/**
 * SPDX-FileCopyrightText: 2021 Aleix Pol i Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2024 Fabian Arndt <fabian.arndt@root-core.net>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "plugin_virtualmonitor_debug.h"
#include <QJsonObject>
#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_VIRTUALMONITOR QStringLiteral("kdeconnect.virtualmonitor")
#define PACKET_TYPE_VIRTUALMONITOR_REQUEST QStringLiteral("kdeconnect.virtualmonitor.request")

class QProcess;

class VirtualMonitorPlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.virtualmonitor")
    Q_PROPERTY(bool hasRemoteVncClient READ hasRemoteVncClient CONSTANT)
    Q_PROPERTY(bool isVirtualMonitorAvailable READ isVirtualMonitorAvailable CONSTANT)

public:
    using KdeConnectPlugin::KdeConnectPlugin;
    ~VirtualMonitorPlugin() override;

    Q_SCRIPTABLE bool requestVirtualMonitor();

    void connected() override;
    QString dbusPath() const override;
    void receivePacket(const NetworkPacket &np) override;

    // The remote device has a VNC client installed
    bool hasRemoteVncClient() const
    {
        return m_capabilitiesRemote.vncClient;
    }

    // krfb is installed on local device, virtual monitors is supported
    bool isVirtualMonitorAvailable() const
    {
        return m_capabilitiesLocal.virtualMonitor;
    }

private:
    void stop();
    bool checkVncClient() const;
    bool checkDefaultSchemeHandler(const QString &scheme) const;

    struct Capabilities {
        bool virtualMonitor = false;
        bool vncClient = false;
    };

    Capabilities m_capabilitiesLocal;
    Capabilities m_capabilitiesRemote;

    QProcess *m_process = nullptr;
    QJsonObject m_remoteResolution;
    uint m_retries = 0;
};
