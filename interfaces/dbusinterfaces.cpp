/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "dbusinterfaces.h"
#include <dbushelper.h>

QString DaemonDbusInterface::activatedService() {
    static const QString service = QStringLiteral("org.kde.kdeconnect");

#ifndef SAILFISHOS
    auto reply = QDBusConnection::sessionBus().interface()->startService(service);
    if (!reply.isValid()) {
        qWarning() << "error activating kdeconnectd:" << reply.error();
    }
#endif

    return service;
}

DaemonDbusInterface::DaemonDbusInterface(QObject* parent)
    : OrgKdeKdeconnectDaemonInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect"), QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDaemonInterface::pairingRequestsChanged, this, &DaemonDbusInterface::pairingRequestsChangedProxy);
    connect(this, &OrgKdeKdeconnectDaemonInterface::customDevicesChanged, this, &DaemonDbusInterface::customDevicesChangedProxy);
}

DaemonDbusInterface::~DaemonDbusInterface()
{

}

DeviceDbusInterface::DeviceDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/") +id, QDBusConnection::sessionBus(), parent)
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
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"), QStringLiteral("/modules/kdeconnect/devices/") +id() + QStringLiteral("/") + plugin, QStringLiteral("org.kde.kdeconnect.device.") + plugin, method);
    QDBusConnection::sessionBus().asyncCall(msg);
}

BatteryDbusInterface::BatteryDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceBatteryInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/") + id + QStringLiteral("/battery"), QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDeviceBatteryInterface::refreshed, this, &BatteryDbusInterface::refreshedProxy);
}

BatteryDbusInterface::~BatteryDbusInterface() = default;

ConnectivityReportDbusInterface::ConnectivityReportDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceConnectivity_reportInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/") + id + QStringLiteral("/connectivity_report"), QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDeviceConnectivity_reportInterface::refreshed, this, &ConnectivityReportDbusInterface::refreshedProxy);
}

ConnectivityReportDbusInterface::~ConnectivityReportDbusInterface() = default;

DeviceNotificationsDbusInterface::DeviceNotificationsDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceNotificationsInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/") + id + QStringLiteral("/notifications"), QDBusConnection::sessionBus(), parent)
{

}

DeviceNotificationsDbusInterface::~DeviceNotificationsDbusInterface()
{

}

NotificationDbusInterface::NotificationDbusInterface(const QString& deviceId, const QString& notificationId, QObject* parent)
    : OrgKdeKdeconnectDeviceNotificationsNotificationInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/") + deviceId + QStringLiteral("/notifications/") + notificationId, QDBusConnection::sessionBus(), parent)
    , id(notificationId)
{

}

NotificationDbusInterface::~NotificationDbusInterface()
{

}

DeviceConversationsDbusInterface::DeviceConversationsDbusInterface(const QString& deviceId, QObject* parent)
    : OrgKdeKdeconnectDeviceConversationsInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/") + deviceId, QDBusConnection::sessionBus(), parent)
{

}

DeviceConversationsDbusInterface::~DeviceConversationsDbusInterface()
{

}

SftpDbusInterface::SftpDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceSftpInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + id + QStringLiteral("/sftp"), QDBusConnection::sessionBus(), parent)
{

}

SftpDbusInterface::~SftpDbusInterface()
{

}

MprisDbusInterface::MprisDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceMprisremoteInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + id + QStringLiteral("/mprisremote"), QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDeviceMprisremoteInterface::propertiesChanged, this, &MprisDbusInterface::propertiesChangedProxy);
}

MprisDbusInterface::~MprisDbusInterface()
{
}

RemoteControlDbusInterface::RemoteControlDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceRemotecontrolInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + id + QStringLiteral("/remotecontrol"), QDBusConnection::sessionBus(), parent)
{
}

RemoteControlDbusInterface::~RemoteControlDbusInterface()
{
}

LockDeviceDbusInterface::LockDeviceDbusInterface(const QString& id, QObject* parent)
    : OrgKdeKdeconnectDeviceLockdeviceInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + id + QStringLiteral("/lockdevice"), QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDeviceLockdeviceInterface::lockedChanged, this, &LockDeviceDbusInterface::lockedChangedProxy);
    Q_ASSERT(isValid());
}

LockDeviceDbusInterface::~LockDeviceDbusInterface()
{
}

FindMyPhoneDeviceDbusInterface::FindMyPhoneDeviceDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceFindmyphoneInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + deviceId + QStringLiteral("/findmyphone"), QDBusConnection::sessionBus(), parent)
{
}

FindMyPhoneDeviceDbusInterface::~FindMyPhoneDeviceDbusInterface()
{
}

RemoteCommandsDbusInterface::RemoteCommandsDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceRemotecommandsInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + deviceId + QStringLiteral("/remotecommands"), QDBusConnection::sessionBus(), parent)
{
}

RemoteCommandsDbusInterface::~RemoteCommandsDbusInterface() = default;

RemoteKeyboardDbusInterface::RemoteKeyboardDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceRemotekeyboardInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + deviceId + QStringLiteral("/remotekeyboard"), QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDeviceRemotekeyboardInterface::remoteStateChanged, this, &RemoteKeyboardDbusInterface::remoteStateChanged);
}

RemoteKeyboardDbusInterface::~RemoteKeyboardDbusInterface() = default;

SmsDbusInterface::SmsDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceSmsInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + deviceId + QStringLiteral("/sms"), QDBusConnection::sessionBus(), parent)
{
}

SmsDbusInterface::~SmsDbusInterface() = default;

ShareDbusInterface::ShareDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceShareInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + deviceId + QStringLiteral("/share"), QDBusConnection::sessionBus(), parent)
{
}

ShareDbusInterface::~ShareDbusInterface() = default;

RemoteSystemVolumeDbusInterface::RemoteSystemVolumeDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceRemotesystemvolumeInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + deviceId + QStringLiteral("/remotesystemvolume"), QDBusConnection::sessionBus(), parent)
{
}

BigscreenDbusInterface::BigscreenDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceBigscreenInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + deviceId + QStringLiteral("/bigscreen"), QDBusConnection::sessionBus(), parent)
{
}

BigscreenDbusInterface::~BigscreenDbusInterface()
{
}

VirtualmonitorDbusInterface::VirtualmonitorDbusInterface(const QString& deviceId, QObject* parent):
    OrgKdeKdeconnectDeviceVirtualmonitorInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect/devices/")  + deviceId + QStringLiteral("/virtualmonitor"), QDBusConnection::sessionBus(), parent)
{
}

VirtualmonitorDbusInterface::~VirtualmonitorDbusInterface()
{
}
