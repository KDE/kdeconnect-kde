/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "dbusinterfaces.h"

QString DaemonDbusInterface::activatedService() {
    static const QString service = QStringLiteral("org.kde.kdeconnect");
    auto reply = QDBusConnection::sessionBus().interface()->startService(service);
    if (!reply.isValid()) {
        qWarning() << "error activating kdeconnectd:" << QDBusConnection::sessionBus().interface()->lastError();
    }
    return service;
}

DaemonDbusInterface::DaemonDbusInterface(QObject* parent)
    : OrgKdeKdeconnectDaemonInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect"), QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDaemonInterface::pairingRequestsChanged, this, &DaemonDbusInterface::pairingRequestsChangedProxy);
}

DaemonDbusInterface::~DaemonDbusInterface()
{

}

DeviceDbusInterface::DeviceDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/"+id, QDBusConnection::sessionBus(), parent)
    , m_id(id)
{
    connect(this, &OrgKdeKdeconnectDeviceInterface::trustedChanged, this, &DeviceDbusInterface::trustedChangedProxy);
    connect(this, &OrgKdeKdeconnectDeviceInterface::reachableChanged, this, &DeviceDbusInterface::reachableChangedProxy);
    connect(this, &OrgKdeKdeconnectDeviceInterface::nameChanged, this, &DeviceDbusInterface::nameChangedProxy);
    connect(this, &OrgKdeKdeconnectDeviceInterface::hasPairingRequestsChanged, this, &DeviceDbusInterface::hasPairingRequestsChangedProxy);
}

DeviceDbusInterface::~DeviceDbusInterface()
{

}

QString DeviceDbusInterface::id() const
{
    return m_id;
}

void DeviceDbusInterface::pluginCall(const QString& plugin, const QString& method)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"), "/modules/kdeconnect/devices/"+id()+'/'+plugin, "org.kde.kdeconnect.device."+plugin, method);
    QDBusConnection::sessionBus().asyncCall(msg);
}

DeviceBatteryDbusInterface::DeviceBatteryDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceBatteryInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/"+id, QDBusConnection::sessionBus(), parent)
{

}

DeviceBatteryDbusInterface::~DeviceBatteryDbusInterface()
{

}

DeviceNotificationsDbusInterface::DeviceNotificationsDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceNotificationsInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/"+id, QDBusConnection::sessionBus(), parent)
{

}

DeviceNotificationsDbusInterface::~DeviceNotificationsDbusInterface()
{

}

NotificationDbusInterface::NotificationDbusInterface(const QString& deviceId, const QString& notificationId, QObject* parent)
    : OrgKdeKdeconnectDeviceNotificationsNotificationInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/"+deviceId+"/notifications/"+notificationId, QDBusConnection::sessionBus(), parent)
    , id(notificationId)
{

}

NotificationDbusInterface::~NotificationDbusInterface()
{

}

SftpDbusInterface::SftpDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceSftpInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/" + id + "/sftp", QDBusConnection::sessionBus(), parent)
{

}

SftpDbusInterface::~SftpDbusInterface()
{

}

MprisDbusInterface::MprisDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceMprisremoteInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/" + id + "/mprisremote", QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDeviceMprisremoteInterface::propertiesChanged, this, &MprisDbusInterface::propertiesChangedProxy);
}

MprisDbusInterface::~MprisDbusInterface()
{
}

RemoteControlDbusInterface::RemoteControlDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceRemotecontrolInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/" + id + "/remotecontrol", QDBusConnection::sessionBus(), parent)
{
}

RemoteControlDbusInterface::~RemoteControlDbusInterface()
{
}

LockDeviceDbusInterface::LockDeviceDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceLockdeviceInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/" + id + "/lockdevice", QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDeviceLockdeviceInterface::lockedChanged, this, &LockDeviceDbusInterface::lockedChangedProxy);
    Q_ASSERT(isValid());
}

LockDeviceDbusInterface::~LockDeviceDbusInterface()
{
}

FindMyPhoneDeviceDbusInterface::FindMyPhoneDeviceDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceFindmyphoneInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/" + deviceId + "/findmyphone", QDBusConnection::sessionBus(), parent)
{
}

FindMyPhoneDeviceDbusInterface::~FindMyPhoneDeviceDbusInterface()
{
}

RemoteCommandsDbusInterface::RemoteCommandsDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceRemotecommandsInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/" + deviceId + "/remotecommands", QDBusConnection::sessionBus(), parent)
{
}

RemoteCommandsDbusInterface::~RemoteCommandsDbusInterface() = default;

RemoteKeyboardDbusInterface::RemoteKeyboardDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceRemotekeyboardInterface(DaemonDbusInterface::activatedService(), "/modules/kdeconnect/devices/" + deviceId + "/remotekeyboard", QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDeviceRemotekeyboardInterface::remoteStateChanged, this, &RemoteKeyboardDbusInterface::remoteStateChanged);
}

RemoteKeyboardDbusInterface::~RemoteKeyboardDbusInterface() = default;

#include "dbusinterfaces.moc"
