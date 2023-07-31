/**
 * SPDX-FileCopyrightText: 2021 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QObject>

#include <core/kdeconnectplugin.h>

#define PACKET_TYPE_LOCK QStringLiteral("kdeconnect.lock")
#define PACKET_TYPE_LOCK_REQUEST QStringLiteral("kdeconnect.lock.request")

class LockDevicePlugin : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.lockdevice")
    Q_PROPERTY(bool isLocked READ isLocked WRITE setLocked NOTIFY lockedChanged)

public:
    explicit LockDevicePlugin(QObject *parent, const QVariantList &args);

    bool isLocked() const;
    Q_SCRIPTABLE void setLocked(bool);

    QString dbusPath() const override;
    void connected() override;
    void receivePacket(const NetworkPacket &np) override;

Q_SIGNALS:
    Q_SCRIPTABLE void lockedChanged(bool locked);

private:
    void sendState();

    bool m_remoteLocked = false;
    bool m_localLocked = false;
};
