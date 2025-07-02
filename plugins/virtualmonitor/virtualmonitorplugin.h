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
    Q_PROPERTY(QString lastError READ lastError)
    Q_PROPERTY(bool isVirtualMonitorAvailable READ isVirtualMonitorAvailable CONSTANT)
    Q_PROPERTY(bool active READ active NOTIFY activeChanged)

public:
    using KdeConnectPlugin::KdeConnectPlugin;
    ~VirtualMonitorPlugin() override;

    Q_SCRIPTABLE bool requestVirtualMonitor();

    /// stops the active session
    Q_SCRIPTABLE void stop();

    QString lastError() const
    {
        return m_lastError;
    }

    void connected() override;
    QString dbusPath() const override;
    void receivePacket(const NetworkPacket &np) override;

    // The remote device has a RDP client installed
    bool hasRemoteClient() const
    {
        return m_capabilitiesRemote.rdpClient;
    }

    // krfb is installed on local device, virtual monitors is supported
    bool isVirtualMonitorAvailable() const
    {
        return m_capabilitiesLocal.virtualMonitor;
    }

    /// Returns true if there's a running virtual monitor
    bool active() const;

Q_SIGNALS:
    Q_SCRIPTABLE void activeChanged();

private:
    bool checkRdpClient() const;
    bool checkDefaultSchemeHandler(const QString &scheme) const;
    bool requestRdp();

    struct Capabilities {
        bool virtualMonitor = false;
        bool rdpClient = false;
    };

    Capabilities m_capabilitiesLocal;
    Capabilities m_capabilitiesRemote;

    QProcess *m_process = nullptr;
    QJsonObject m_remoteResolution;
    uint m_retries = 0;
    QString m_lastError;
};
