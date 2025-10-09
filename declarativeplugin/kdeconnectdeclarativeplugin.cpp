/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "kdeconnectdeclarativeplugin.h"

#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <devicespluginfilterproxymodel.h>

#include "objectfactory.h"
#include "responsewaiter.h"

#include "core/kdeconnectpluginconfig.h"
#include "interfaces/commandsmodel.h"
#include "interfaces/devicesmodel.h"
#include "interfaces/devicessortproxymodel.h"
#include "interfaces/notificationsmodel.h"
#include "openconfig.h"
#include "pointerlocker.h"
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
#include "pointerlockerwayland.h"
#endif
#include <pluginmodel.h>
#include <remotecommandsmodel.h>
#include <remotesinksmodel.h>

QObject *createDBusResponse()
{
    return new DBusAsyncResponse();
}

template<typename T>
void registerFactory(const char *uri, const char *name)
{
    qmlRegisterSingletonType<ObjectFactory>(uri, 1, 0, name, [](QQmlEngine *engine, QJSEngine *) -> QObject * {
        return new ObjectFactory(engine, [](const QVariant &deviceId) -> QObject * {
            return new T(deviceId.toString());
        });
    });
}

void KdeConnectDeclarativePlugin::registerTypes(const char *uri)
{
    qmlRegisterType<DevicesModel>(uri, 1, 0, "DevicesModel");
    qmlRegisterType<NotificationsModel>(uri, 1, 0, "NotificationsModel");
    qmlRegisterType<RemoteCommandsModel>(uri, 1, 0, "RemoteCommandsModel");
    qmlRegisterType<DBusAsyncResponse>(uri, 1, 0, "DBusAsyncResponse");
    qmlRegisterType<DevicesSortProxyModel>(uri, 1, 0, "DevicesSortProxyModel");
    qmlRegisterType<DevicesPluginFilterProxyModel>(uri, 1, 0, "DevicesPluginFilterProxyModel");
    qmlRegisterType<RemoteSinksModel>(uri, 1, 0, "RemoteSinksModel");
    qmlRegisterType<PluginModel>(uri, 1, 0, "PluginModel");
    qmlRegisterType<KdeConnectPluginConfig>(uri, 1, 0, "KdeConnectPluginConfig");
    qmlRegisterType<CommandsModel>(uri, 1, 0, "CommandsModel");
    qmlRegisterUncreatableType<MprisDbusInterface>(uri, 1, 0, "MprisDbusInterface", QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<LockDeviceDbusInterface>(uri, 1, 0, "LockDeviceDbusInterface", QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<FindMyPhoneDeviceDbusInterface>(uri,
                                                               1,
                                                               0,
                                                               "FindMyPhoneDbusInterface",
                                                               QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<ClipboardDbusInterface>(uri, 1, 0, "ClipboardDbusInterface", QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<RemoteKeyboardDbusInterface>(uri,
                                                            1,
                                                            0,
                                                            "RemoteKeyboardDbusInterface",
                                                            QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<DeviceDbusInterface>(uri, 1, 0, "DeviceDbusInterface", QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<BatteryDbusInterface>(uri, 1, 0, "BatteryDbusInterface", QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<ConnectivityReportDbusInterface>(uri,
                                                                1,
                                                                0,
                                                                "ConnectivityReportDbusInterface",
                                                                QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<SftpDbusInterface>(uri, 1, 0, "SftpDbusInterface", QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<SmsDbusInterface>(uri, 1, 0, "SmsDbusInterface", QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<VirtualmonitorDbusInterface>(uri,
                                                            1,
                                                            0,
                                                            "VirtualmonitorDbusInterface",
                                                            QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<RemoteCommandsDbusInterface>(uri,
                                                            1,
                                                            0,
                                                            "RemoteCommandsDbusInterface",
                                                            QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<RemoteSystemVolumeDbusInterface>(uri,
                                                                1,
                                                                0,
                                                                "RemoteSystemVolumeInterface",
                                                                QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterUncreatableType<ShareDbusInterface>(uri, 1, 0, "ShareDbusInterface", QStringLiteral("You're not supposed to instantiate interfaces"));
    qmlRegisterSingletonType<DaemonDbusInterface>(uri, 1, 0, "DaemonDbusInterface", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return new DaemonDbusInterface;
    });
    qmlRegisterSingletonType<AbstractPointerLocker>("org.kde.kdeconnect", 1, 0, "PointerLocker", [](QQmlEngine *, QJSEngine *) -> QObject * {
        AbstractPointerLocker *ret;
#if defined(Q_OS_LINUX) || defined(Q_OS_FREEBSD)
        if (qGuiApp->platformName() == QLatin1String("wayland"))
            ret = new PointerLockerWayland;
        else
#endif
            ret = new PointerLockerQt;
        return ret;
    });

    qmlRegisterSingletonType<OpenConfig>(uri, 1, 0, "OpenConfig", [](QQmlEngine *, QJSEngine *) -> QObject * {
        return new OpenConfig;
    });

    qmlRegisterAnonymousType<QAbstractItemModel>(uri, 1);

    registerFactory<DeviceDbusInterface>(uri, "DeviceDbusInterfaceFactory");
    registerFactory<BatteryDbusInterface>(uri, "DeviceBatteryDbusInterfaceFactory");
    registerFactory<ConnectivityReportDbusInterface>(uri, "DeviceConnectivityReportDbusInterfaceFactory");
    registerFactory<FindMyPhoneDeviceDbusInterface>(uri, "FindMyPhoneDbusInterfaceFactory");
    registerFactory<SftpDbusInterface>(uri, "SftpDbusInterfaceFactory");
    registerFactory<RemoteKeyboardDbusInterface>(uri, "RemoteKeyboardDbusInterfaceFactory");
    registerFactory<ClipboardDbusInterface>(uri, "ClipboardDbusInterfaceFactory");
    registerFactory<MprisDbusInterface>(uri, "MprisDbusInterfaceFactory");
    registerFactory<RemoteControlDbusInterface>(uri, "RemoteControlDbusInterfaceFactory");
    registerFactory<LockDeviceDbusInterface>(uri, "LockDeviceDbusInterfaceFactory");
    registerFactory<SmsDbusInterface>(uri, "SmsDbusInterfaceFactory");
    registerFactory<RemoteCommandsDbusInterface>(uri, "RemoteCommandsDbusInterfaceFactory");
    registerFactory<ShareDbusInterface>(uri, "ShareDbusInterfaceFactory");
    registerFactory<RemoteSystemVolumeDbusInterface>(uri, "RemoteSystemVolumeDbusInterfaceFactory");
    registerFactory<VirtualmonitorDbusInterface>(uri, "VirtualmonitorDbusInterfaceFactory");
}

void KdeConnectDeclarativePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);

    engine->rootContext()->setContextProperty(QStringLiteral("DBusResponseFactory"), new ObjectFactory(engine, createDBusResponse));

    engine->rootContext()->setContextProperty(QStringLiteral("DBusResponseWaiter"), DBusResponseWaiter::instance());
}

#include "moc_kdeconnectdeclarativeplugin.cpp"
