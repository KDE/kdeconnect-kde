/**
 * Copyright 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "lockdeviceplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QLoggingCategory>
#include "screensaverdbusinterface.h"

#include <core/device.h>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectLockPluginFactory, "kdeconnect_lockdevice.json", registerPlugin<LockDevicePlugin>(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_LOCKREMOTE, "kdeconnect.plugin.lock")

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
    NetworkPackage np(PACKAGE_TYPE_LOCK);
    np.set("setLocked", locked);
    sendPackage(np);
}

bool LockDevicePlugin::receivePackage(const NetworkPackage & np)
{
    if (np.has("isLocked")) {
        bool locked = np.get<bool>("isLocked");
        if (m_remoteLocked != locked) {
            m_remoteLocked = locked;
            Q_EMIT lockedChanged(locked);
        }
    }

    bool sendState = np.has("requestLocked");
    if (np.has("setLocked")) {
        iface()->SetActive(np.get<bool>("setLocked"));
        sendState = true;
    }
    if (sendState) {
        NetworkPackage np(PACKAGE_TYPE_LOCK);
        np.set("isLocked", iface()->GetActive());
        sendPackage(np);
    }

    return true;
}

OrgFreedesktopScreenSaverInterface* LockDevicePlugin::iface()
{
    if (!m_iface) {
        m_iface = new OrgFreedesktopScreenSaverInterface("org.freedesktop.ScreenSaver", "/org/freedesktop/ScreenSaver", QDBusConnection::sessionBus());
        if(!m_iface->isValid())
            qCWarning(KDECONNECT_PLUGIN_LOCKREMOTE) << "Couldn't connect to the ScreenSaver interface";
    }
    return m_iface;
}

void LockDevicePlugin::connected()
{
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportAllContents);

    NetworkPackage np(PACKAGE_TYPE_LOCK);
    np.set("requestLocked", QVariant());
    sendPackage(np);
}

QString LockDevicePlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/lockdevice";
}

#include "lockdeviceplugin.moc"
