/**
 * SPDX-FileCopyrightText: 2021 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "lockdeviceplugin-win.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <QDebug>

#include <core/daemon.h>
#include <core/device.h>
#include <dbushelper.h>

#include <Windows.h>

K_PLUGIN_CLASS_WITH_JSON(LockDevicePlugin, "kdeconnect_lockdevice.json")

LockDevicePlugin::LockDevicePlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_remoteLocked(false)
{
}

bool LockDevicePlugin::isLocked() const
{
    return m_remoteLocked; // Windows doesn't support monitoring lock status, m_remoteLocked is never updated
}

void LockDevicePlugin::setLocked(bool locked)
{
    NetworkPacket np(PACKET_TYPE_LOCK_REQUEST, {{QStringLiteral("setLocked"), locked}});
    sendPacket(np);
}

void LockDevicePlugin::receivePacket(const NetworkPacket &np)
{
    if (np.has(QStringLiteral("isLocked"))) {
        bool locked = np.get<bool>(QStringLiteral("isLocked"));
        if (m_remoteLocked != locked) {
            m_remoteLocked = locked;
            Q_EMIT lockedChanged(locked);
        }
    }

    if (np.has(QStringLiteral("requestLocked"))) {
        sendState();
    }

    // Receiving result of setLocked
    if (np.has(QStringLiteral("lockResult"))) {
        bool lockSuccess = np.get<bool>(QStringLiteral("lockResult"));
        if (lockSuccess) {
            Daemon::instance()->sendSimpleNotification(QStringLiteral("remoteLockSuccess"),
                                                       device()->name(),
                                                       i18n("Remote lock successful"),
                                                       QStringLiteral("error"));
        } else {
            Daemon::instance()->sendSimpleNotification(QStringLiteral("remoteLockFail"), device()->name(), i18n("Remote lock failed"), QStringLiteral("error"));
            Daemon::instance()->reportError(device()->name(), i18n("Remote lock failed"));
        }
    }

    if (np.has(QStringLiteral("setLocked"))) {
        const bool lock = np.get<bool>(QStringLiteral("setLocked"));
        bool success = false;
        if (lock) {
            success = LockWorkStation();
            NetworkPacket np(PACKET_TYPE_LOCK, {{QStringLiteral("lockResult"), success}});
            sendPacket(np);
        }

        sendState();
    }
}

void LockDevicePlugin::sendState()
{
    NetworkPacket np(PACKET_TYPE_LOCK, {{QStringLiteral("isLocked"), m_localLocked}});
    sendPacket(np);
}

void LockDevicePlugin::connected()
{
    NetworkPacket np(PACKET_TYPE_LOCK_REQUEST, {{QStringLiteral("requestLocked"), QVariant()}});
    sendPacket(np);
}

QString LockDevicePlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/lockdevice").arg(device()->id());
}

#include "lockdeviceplugin-win.moc"
#include "moc_lockdeviceplugin-win.cpp"
