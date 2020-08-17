/**
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "lockdeviceplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include "screensaverdbusinterface.h"
#include "plugin_lock_debug.h"

#include <core/device.h>
#include <dbushelper.h>

K_PLUGIN_CLASS_WITH_JSON(LockDevicePlugin, "kdeconnect_lockdevice.json")

LockDevicePlugin::LockDevicePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , m_remoteLocked(false)
    , m_iface(nullptr)
{
}

LockDevicePlugin::~LockDevicePlugin()
{
    delete m_iface;
}

bool LockDevicePlugin::isLocked() const
{
    return m_remoteLocked;
}
void LockDevicePlugin::setLocked(bool locked)
{
    NetworkPacket np(PACKET_TYPE_LOCK_REQUEST, {{QStringLiteral("setLocked"), locked}});
    sendPacket(np);
}

bool LockDevicePlugin::receivePacket(const NetworkPacket & np)
{
    if (np.has(QStringLiteral("isLocked"))) {
        bool locked = np.get<bool>(QStringLiteral("isLocked"));
        if (m_remoteLocked != locked) {
            m_remoteLocked = locked;
            Q_EMIT lockedChanged(locked);
        }
    }

    bool sendState = np.has(QStringLiteral("requestLocked"));
    if (np.has(QStringLiteral("setLocked"))) {
        iface()->SetActive(np.get<bool>(QStringLiteral("setLocked")));
        sendState = true;
    }
    if (sendState) {
        NetworkPacket np(PACKET_TYPE_LOCK, QVariantMap {{QStringLiteral("isLocked"), QVariant::fromValue<bool>(iface()->GetActive())}});
        sendPacket(np);
    }

    return true;
}

OrgFreedesktopScreenSaverInterface* LockDevicePlugin::iface()
{
    if (!m_iface) {
        m_iface = new OrgFreedesktopScreenSaverInterface(QStringLiteral("org.freedesktop.ScreenSaver"), QStringLiteral("/org/freedesktop/ScreenSaver"), DBusHelper::sessionBus());
        if(!m_iface->isValid())
            qCWarning(KDECONNECT_PLUGIN_LOCKREMOTE) << "Couldn't connect to the ScreenSaver interface";
    }
    return m_iface;
}

void LockDevicePlugin::connected()
{
    NetworkPacket np(PACKET_TYPE_LOCK_REQUEST, {{QStringLiteral("requestLocked"), QVariant()}});
    sendPacket(np);
}

QString LockDevicePlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/lockdevice");
}

#include "lockdeviceplugin.moc"
