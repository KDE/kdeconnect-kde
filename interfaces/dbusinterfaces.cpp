/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "dbusinterfaces.h"

QString DaemonDbusInterface::activatedService()
{
    static const QString service = QStringLiteral("org.kde.kdeconnect");

    auto reply = QDBusConnection::sessionBus().interface()->startService(service);
    if (!reply.isValid()) {
        qWarning() << "error activating kdeconnectd:" << reply.error();
    }

    return service;
}

DaemonDbusInterface::DaemonDbusInterface(QObject *parent)
    : OrgKdeKdeconnectDaemonInterface(DaemonDbusInterface::activatedService(), QStringLiteral("/modules/kdeconnect"), QDBusConnection::sessionBus(), parent)
{
    connect(this, &OrgKdeKdeconnectDaemonInterface::customDevicesChanged, this, &DaemonDbusInterface::customDevicesChangedProxy);
}

DeviceDbusInterface::DeviceDbusInterface(const QString &id, QObject *parent)
    : OrgKdeKdeconnectDeviceInterface(DaemonDbusInterface::activatedService(),
                                      QStringLiteral("/modules/kdeconnect/devices/") + id,
                                      QDBusConnection::sessionBus(),
                                      parent)
    , m_id(id)
{
    connect(this, &OrgKdeKdeconnectDeviceInterface::pairStateChanged, this, &DeviceDbusInterface::pairStateChangedProxy);
    connect(this, &OrgKdeKdeconnectDeviceInterface::reachableChanged, this, &DeviceDbusInterface::reachableChangedProxy);
    connect(this, &OrgKdeKdeconnectDeviceInterface::nameChanged, this, &DeviceDbusInterface::nameChangedProxy);
}

QString DeviceDbusInterface::id() const
{
    return m_id;
}

void DeviceDbusInterface::pluginCall(const QString &plugin, const QString &method)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.kdeconnect"),
                                                      QStringLiteral("/modules/kdeconnect/devices/") + id() + QStringLiteral("/") + plugin,
                                                      QStringLiteral("org.kde.kdeconnect.device.") + plugin,
                                                      method);
    QDBusConnection::sessionBus().asyncCall(msg);
}

BatteryDbusInterface::BatteryDbusInterface(const QString &id, QObject *parent)
    : OrgKdeKdeconnectDeviceBatteryInterface(DaemonDbusInterface::activatedService(),
                                             QLatin1String("/modules/kdeconnect/devices/%1/battery").arg(id),
                                             QDBusConnection::sessionBus(),
                                             parent)
{
    connect(this, &OrgKdeKdeconnectDeviceBatteryInterface::refreshed, this, &BatteryDbusInterface::refreshedProxy);
}

ConnectivityReportDbusInterface::ConnectivityReportDbusInterface(const QString &id, QObject *parent)
    : OrgKdeKdeconnectDeviceConnectivity_reportInterface(DaemonDbusInterface::activatedService(),
                                                         QLatin1String("/modules/kdeconnect/devices/%1/connectivity_report").arg(id),
                                                         QDBusConnection::sessionBus(),
                                                         parent)
{
    connect(this, &OrgKdeKdeconnectDeviceConnectivity_reportInterface::refreshed, this, &ConnectivityReportDbusInterface::refreshedProxy);
}

DeviceNotificationsDbusInterface::DeviceNotificationsDbusInterface(const QString &id, QObject *parent)
    : OrgKdeKdeconnectDeviceNotificationsInterface(DaemonDbusInterface::activatedService(),
                                                   QLatin1String("/modules/kdeconnect/devices/%1/notifications").arg(id),
                                                   QDBusConnection::sessionBus(),
                                                   parent)
{
}

NotificationDbusInterface::NotificationDbusInterface(const QString &deviceId, const QString &notificationId, QObject *parent)
    : OrgKdeKdeconnectDeviceNotificationsNotificationInterface(DaemonDbusInterface::activatedService(),
                                                               QLatin1String("/modules/kdeconnect/devices/%1/notifications/").arg(deviceId) + notificationId,
                                                               QDBusConnection::sessionBus(),
                                                               parent)
    , id(notificationId)
{
}

DeviceConversationsDbusInterface::DeviceConversationsDbusInterface(const QString &deviceId, QObject *parent)
    : OrgKdeKdeconnectDeviceConversationsInterface(DaemonDbusInterface::activatedService(),
                                                   QStringLiteral("/modules/kdeconnect/devices/") + deviceId,
                                                   QDBusConnection::sessionBus(),
                                                   parent)
{
}

SftpDbusInterface::SftpDbusInterface(const QString &id, QObject *parent)
    : OrgKdeKdeconnectDeviceSftpInterface(DaemonDbusInterface::activatedService(),
                                          QLatin1String("/modules/kdeconnect/devices/%1/sftp").arg(id),
                                          QDBusConnection::sessionBus(),
                                          parent)
{
}

MprisDbusInterface::MprisDbusInterface(const QString &id, QObject *parent)
    : OrgKdeKdeconnectDeviceMprisremoteInterface(DaemonDbusInterface::activatedService(),
                                                 QLatin1String("/modules/kdeconnect/devices/%1/mprisremote").arg(id),
                                                 QDBusConnection::sessionBus(),
                                                 parent)
{
    connect(this, &OrgKdeKdeconnectDeviceMprisremoteInterface::propertiesChanged, this, &MprisDbusInterface::propertiesChangedProxy);
}

RemoteControlDbusInterface::RemoteControlDbusInterface(const QString &id, QObject *parent)
    : OrgKdeKdeconnectDeviceRemotecontrolInterface(DaemonDbusInterface::activatedService(),
                                                   QLatin1String("/modules/kdeconnect/devices/%1/remotecontrol").arg(id),
                                                   QDBusConnection::sessionBus(),
                                                   parent)
{
}

LockDeviceDbusInterface::LockDeviceDbusInterface(const QString &id, QObject *parent)
    : OrgKdeKdeconnectDeviceLockdeviceInterface(DaemonDbusInterface::activatedService(),
                                                QLatin1String("/modules/kdeconnect/devices/%1/lockdevice").arg(id),
                                                QDBusConnection::sessionBus(),
                                                parent)
{
    connect(this, &OrgKdeKdeconnectDeviceLockdeviceInterface::lockedChanged, this, &LockDeviceDbusInterface::lockedChangedProxy);
    Q_ASSERT(isValid());
}

FindMyPhoneDeviceDbusInterface::FindMyPhoneDeviceDbusInterface(const QString &deviceId, QObject *parent)
    : OrgKdeKdeconnectDeviceFindmyphoneInterface(DaemonDbusInterface::activatedService(),
                                                 QLatin1String("/modules/kdeconnect/devices/%1/findmyphone").arg(deviceId),
                                                 QDBusConnection::sessionBus(),
                                                 parent)
{
}

RemoteCommandsDbusInterface::RemoteCommandsDbusInterface(const QString &deviceId, QObject *parent)
    : OrgKdeKdeconnectDeviceRemotecommandsInterface(DaemonDbusInterface::activatedService(),
                                                    QLatin1String("/modules/kdeconnect/devices/%1/remotecommands").arg(deviceId),
                                                    QDBusConnection::sessionBus(),
                                                    parent)
{
}

RemoteKeyboardDbusInterface::RemoteKeyboardDbusInterface(const QString &deviceId, QObject *parent)
    : OrgKdeKdeconnectDeviceRemotekeyboardInterface(DaemonDbusInterface::activatedService(),
                                                    QLatin1String("/modules/kdeconnect/devices/%1/remotekeyboard").arg(deviceId),
                                                    QDBusConnection::sessionBus(),
                                                    parent)
{
    connect(this, &OrgKdeKdeconnectDeviceRemotekeyboardInterface::remoteStateChanged, this, &RemoteKeyboardDbusInterface::remoteStateChangedProxy);
}

SmsDbusInterface::SmsDbusInterface(const QString &deviceId, QObject *parent)
    : OrgKdeKdeconnectDeviceSmsInterface(DaemonDbusInterface::activatedService(),
                                         QLatin1String("/modules/kdeconnect/devices/%1/sms").arg(deviceId),
                                         QDBusConnection::sessionBus(),
                                         parent)
{
}

ShareDbusInterface::ShareDbusInterface(const QString &deviceId, QObject *parent)
    : OrgKdeKdeconnectDeviceShareInterface(DaemonDbusInterface::activatedService(),
                                           QLatin1String("/modules/kdeconnect/devices/%1/share").arg(deviceId),
                                           QDBusConnection::sessionBus(),
                                           parent)
{
}

RemoteSystemVolumeDbusInterface::RemoteSystemVolumeDbusInterface(const QString &deviceId, QObject *parent)
    : OrgKdeKdeconnectDeviceRemotesystemvolumeInterface(DaemonDbusInterface::activatedService(),
                                                        QLatin1String("/modules/kdeconnect/devices/%1/remotesystemvolume").arg(deviceId),
                                                        QDBusConnection::sessionBus(),
                                                        parent)
{
}

BigscreenDbusInterface::BigscreenDbusInterface(const QString &deviceId, QObject *parent)
    : OrgKdeKdeconnectDeviceBigscreenInterface(DaemonDbusInterface::activatedService(),
                                               QLatin1String("/modules/kdeconnect/devices/%1/bigscreen").arg(deviceId),
                                               QDBusConnection::sessionBus(),
                                               parent)
{
}

VirtualmonitorDbusInterface::VirtualmonitorDbusInterface(const QString &deviceId, QObject *parent)
    : OrgKdeKdeconnectDeviceVirtualmonitorInterface(DaemonDbusInterface::activatedService(),
                                                    QLatin1String("/modules/kdeconnect/devices/%1/virtualmonitor").arg(deviceId),
                                                    QDBusConnection::sessionBus(),
                                                    parent)
{
    connect(this, &OrgKdeKdeconnectDeviceVirtualmonitorInterface::activeChanged, this, &VirtualmonitorDbusInterface::activeChanged);
}

ClipboardDbusInterface::ClipboardDbusInterface(const QString &deviceId, QObject *parent)
    : OrgKdeKdeconnectDeviceClipboardInterface(DaemonDbusInterface::activatedService(),
                                               QLatin1String("/modules/kdeconnect/devices/%1/clipboard").arg(deviceId),
                                               QDBusConnection::sessionBus(),
                                               parent)
{
    connect(this, &OrgKdeKdeconnectDeviceClipboardInterface::autoShareDisabledChanged, this, &ClipboardDbusInterface::autoShareDisabledChangedProxy);
}

#include "moc_dbusinterfaces.cpp"
