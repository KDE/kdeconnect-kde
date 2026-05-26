/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECTDECLARATIVEPLUGIN_H
#define KDECONNECTDECLARATIVEPLUGIN_H

#include <QGuiApplication>
#include <QQmlEngine>
#include <QQmlExtensionPlugin>

#include <qqmlregistration.h>

#include "commandsmodel.h"
#include "dbusinterfaces.h"
#include "devicesmodel.h"
#include "devicespluginfilterproxymodel.h"
#include "devicessortproxymodel.h"
#include "kdeconnectpluginconfig.h"
#include "notificationsmodel.h"
#include "openconfig.h"
#include "pluginmodel.h"
#include "pointerlocker.h"
#include "remotecommandsmodel.h"
#include "remotesinksmodel.h"
#include "responsewaiter.h"

#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include "pointerlockerwayland.h"
#endif

class KdeConnectPluginConfigForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(KdeConnectPluginConfig)
    QML_FOREIGN(KdeConnectPluginConfig)
};

class DBusAsyncResponseForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(DBusAsyncResponse)
    QML_FOREIGN(DBusAsyncResponse)
};

class CommandsModelForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(CommandsModel)
    QML_FOREIGN(CommandsModel)
};

class PluginModelForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(PluginModel)
    QML_FOREIGN(PluginModel)
};

class DevicesModelForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(DevicesModel)
    QML_FOREIGN(DevicesModel)
};

class DevicesSortProxyModelForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(DevicesSortProxyModel)
    QML_FOREIGN(DevicesSortProxyModel)
};

class DevicesPluginFilterProxyModelForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(DevicesPluginFilterProxyModel)
    QML_FOREIGN(DevicesPluginFilterProxyModel)
};

class NotificationsModelForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(NotificationsModel)
    QML_FOREIGN(NotificationsModel)
};

class RemoteCommandsModelForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(RemoteCommandsModel)
    QML_FOREIGN(RemoteCommandsModel)
};

class RemoteSinksModelForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(RemoteSinksModel)
    QML_FOREIGN(RemoteSinksModel)
};

class BatteryDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(BatteryDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(BatteryDbusInterface)
};

class DeviceDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(DeviceDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(DeviceDbusInterface)
};

class ConnectivityReportDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(ConnectivityReportDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(ConnectivityReportDbusInterface)
};

class ShareDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(ShareDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(ShareDbusInterface)
};

class VirtualmonitorDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(VirtualmonitorDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(VirtualmonitorDbusInterface)
};

class SftpDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(SftpDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(SftpDbusInterface)
};

class ClipboardDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(ClipboardDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(ClipboardDbusInterface)
};

class RemoteKeyboardDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(RemoteKeyboardDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(RemoteKeyboardDbusInterface)
};

class MprisDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(MprisDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(MprisDbusInterface)
};

class LockDeviceDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(LockDeviceDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(LockDeviceDbusInterface)
};

class FindMyPhoneDeviceDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(FindMyPhoneDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(FindMyPhoneDeviceDbusInterface)
};

class SmsDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(SmsDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(SmsDbusInterface)
};

class RemoteCommandsDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(RemoteCommandsDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(RemoteCommandsDbusInterface)
};

class RemoteSystemVolumeDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(RemoteSystemVolumeDbusInterface)
    QML_UNCREATABLE("")
    QML_FOREIGN(RemoteSystemVolumeDbusInterface)
};

class DaemonDbusInterfaceForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(DaemonDbusInterface)
    QML_SINGLETON
    QML_FOREIGN(DaemonDbusInterface)
};

class OpenConfigForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(OpenConfig)
    QML_SINGLETON
    QML_FOREIGN(OpenConfig)
};

class DeviceDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE DeviceDbusInterface *create(const QString &deviceId)
    {
        return new DeviceDbusInterface(deviceId);
    }
};

class DeviceBatteryDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE BatteryDbusInterface *create(const QString &deviceId)
    {
        return new BatteryDbusInterface(deviceId);
    }
};

class DeviceConnectivityReportDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE ConnectivityReportDbusInterface *create(const QString &deviceId)
    {
        return new ConnectivityReportDbusInterface(deviceId);
    }
};

class ShareDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE ShareDbusInterface *create(const QString &deviceId)
    {
        return new ShareDbusInterface(deviceId);
    }
};

class VirtualmonitorDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE VirtualmonitorDbusInterface *create(const QString &deviceId)
    {
        return new VirtualmonitorDbusInterface(deviceId);
    }
};

class SftpDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE SftpDbusInterface *create(const QString &deviceId)
    {
        return new SftpDbusInterface(deviceId);
    }
};

class SmsDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE SmsDbusInterface *create(const QString &deviceId)
    {
        return new SmsDbusInterface(deviceId);
    }
};

class ClipboardDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE ClipboardDbusInterface *create(const QString &deviceId)
    {
        return new ClipboardDbusInterface(deviceId);
    }
};

class RemoteSystemVolumeDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE RemoteSystemVolumeDbusInterface *create(const QString &deviceId)
    {
        return new RemoteSystemVolumeDbusInterface(deviceId);
    }
};

class RemoteKeyboardDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE RemoteKeyboardDbusInterface *create(const QString &deviceId)
    {
        return new RemoteKeyboardDbusInterface(deviceId);
    }
};

class RemoteControlDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE RemoteControlDbusInterface *create(const QString &deviceId)
    {
        return new RemoteControlDbusInterface(deviceId);
    }
};

class LockDeviceDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE LockDeviceDbusInterface *create(const QString &deviceId)
    {
        return new LockDeviceDbusInterface(deviceId);
    }
};

class FindMyPhoneDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE FindMyPhoneDeviceDbusInterface *create(const QString &deviceId)
    {
        return new FindMyPhoneDeviceDbusInterface(deviceId);
    }
};

class RemoteCommandsDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE RemoteCommandsDbusInterface *create(const QString &deviceId)
    {
        return new RemoteCommandsDbusInterface(deviceId);
    }
};

class MprisDbusInterfaceFactory : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
public:
    Q_INVOKABLE MprisDbusInterface *create(const QString &deviceId)
    {
        return new MprisDbusInterface(deviceId);
    }
};

class AbstractPointerLockerForeign
{
    Q_GADGET
    QML_NAMED_ELEMENT(PointerLocker)
    QML_FOREIGN(AbstractPointerLocker)
    QML_SINGLETON

public:
    static AbstractPointerLocker *create(QQmlEngine *, QJSEngine *)
    {
        AbstractPointerLocker *ret;
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
        if (qGuiApp->platformName() == QLatin1String("wayland"))
            ret = new PointerLockerWayland;
        else
#endif
            ret = new PointerLockerQt;
        return ret;
    }

private:
    AbstractPointerLockerForeign() = default;
};

class KdeConnectDeclarativePlugin : public QQmlExtensionPlugin
{
    Q_OBJECT

    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

    void registerTypes(const char *uri) override;
    void initializeEngine(QQmlEngine *engine, const char *uri) override;
};

#endif // KDECONNECTDECLARATIVEPLUGIN_H
