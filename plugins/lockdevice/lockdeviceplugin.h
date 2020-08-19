/**
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef LOCKDEVICEPLUGIN_H
#define LOCKDEVICEPLUGIN_H

#include <QObject>

#include <core/kdeconnectplugin.h>

#include "login1dbusinterface.h"
#include "propertiesdbusinterface.h"

#define PACKET_TYPE_LOCK QStringLiteral("kdeconnect.lock")
#define PACKET_TYPE_LOCK_REQUEST QStringLiteral("kdeconnect.lock.request")

class Q_DECL_EXPORT LockDevicePlugin
    : public KdeConnectPlugin
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.kdeconnect.device.lockdevice")
    Q_PROPERTY(bool isLocked READ isLocked WRITE setLocked NOTIFY lockedChanged)

public:
    explicit LockDevicePlugin(QObject* parent, const QVariantList &args);
    ~LockDevicePlugin() override;

    bool isLocked() const;
    void setLocked(bool b);

    QString dbusPath() const override;
    void connected() override;
    bool receivePacket(const NetworkPacket & np) override;

Q_SIGNALS:
    Q_SCRIPTABLE void lockedChanged(bool locked);

private:

    void sendState();

    bool m_remoteLocked;
    bool m_localLocked = false;

    OrgFreedesktopLogin1SessionInterface m_login1Interface;
    OrgFreedesktopDBusPropertiesInterface m_propertiesInterface;
};

#endif
