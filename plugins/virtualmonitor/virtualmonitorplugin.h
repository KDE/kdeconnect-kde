/**
 * SPDX-FileCopyrightText: 2021 Aleix Pol i Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef VIRTUALMONITORPLUGIN_H
#define VIRTUALMONITORPLUGIN_H

#include <core/kdeconnectplugin.h>
#include <QJsonObject>
#include "plugin_virtualmonitor_debug.h"

#define PACKET_TYPE_VIRTUALMONITOR QStringLiteral("kdeconnect.virtualmonitor")
#define PACKET_TYPE_VIRTUALMONITOR_REQUEST QStringLiteral("kdeconnect.virtualmonitor.request")

class QProcess;

class VirtualMonitorPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.virtualmonitor")

public:
    explicit VirtualMonitorPlugin(QObject* parent, const QVariantList& args);
    ~VirtualMonitorPlugin() override;

    Q_SCRIPTABLE bool requestVirtualMonitor();

    void connected() override;
    QString dbusPath() const override;
    bool receivePacket(const NetworkPacket& np) override;

private:
    void stop();

    QProcess *m_process = nullptr;
    QJsonObject m_remoteResolution;
    uint m_retries = 0;
};

#endif
