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

#include "kdeconnectdeclarativeplugin.h"

#include <QQmlEngine>
#include <QQmlContext>
#include <QDBusPendingCall>
#include <QDBusPendingReply>
#include <QtQml>

#include "objectfactory.h"
#include "responsewaiter.h"

#include "interfaces/devicessortproxymodel.h"
#include "interfaces/devicesmodel.h"
#include "interfaces/notificationsmodel.h"

QObject* createDeviceDbusInterface(const QVariant& deviceId)
{
    return new DeviceDbusInterface(deviceId.toString());
}

QObject* createDeviceBatteryDbusInterface(const QVariant& deviceId)
{
    return new DeviceBatteryDbusInterface(deviceId.toString());
}

QObject* createFindMyPhoneInterface(const QVariant& deviceId)
{
    return new FindMyPhoneDeviceDbusInterface(deviceId.toString());
}

QObject* createRemoteKeyboardInterface(const QVariant& deviceId)
{
    return new RemoteKeyboardDbusInterface(deviceId.toString());
}

QObject* createSftpInterface(const QVariant& deviceId)
{
    return new SftpDbusInterface(deviceId.toString());
}

QObject* createRemoteControlInterface(const QVariant& deviceId)
{
    return new RemoteControlDbusInterface(deviceId.toString());
}

QObject* createMprisInterface(const QVariant& deviceId)
{
    return new MprisDbusInterface(deviceId.toString());
}

QObject* createDeviceLockInterface(const QVariant& deviceId)
{
    Q_ASSERT(!deviceId.toString().isEmpty());
    return new LockDeviceDbusInterface(deviceId.toString());
}

QObject* createDBusResponse()
{
    return new DBusAsyncResponse();
}

void KdeConnectDeclarativePlugin::registerTypes(const char* uri)
{
    qmlRegisterType<DevicesModel>(uri, 1, 0, "DevicesModel");
    qmlRegisterType<NotificationsModel>(uri, 1, 0, "NotificationsModel");
    qmlRegisterType<DBusAsyncResponse>(uri, 1, 0, "DBusAsyncResponse");
    qmlRegisterType<DevicesSortProxyModel>(uri, 1, 0, "DevicesSortProxyModel");
    qmlRegisterUncreatableType<MprisDbusInterface>(uri, 1, 0, "MprisDbusInterface", QStringLiteral("You're not supposed to instantiate interfacess"));
    qmlRegisterUncreatableType<LockDeviceDbusInterface>(uri, 1, 0, "LockDeviceDbusInterface", QStringLiteral("You're not supposed to instantiate interfacess"));
    qmlRegisterUncreatableType<FindMyPhoneDeviceDbusInterface>(uri, 1, 0, "FindMyPhoneDbusInterface", QStringLiteral("You're not supposed to instantiate interfacess"));
    qmlRegisterUncreatableType<RemoteKeyboardDbusInterface>(uri, 1, 0, "RemoteKeyboardDbusInterface", QStringLiteral("You're not supposed to instantiate interfacess"));
    qmlRegisterUncreatableType<DeviceDbusInterface>(uri, 1, 0, "DeviceDbusInterface", QStringLiteral("You're not supposed to instantiate interfacess"));
    qmlRegisterSingletonType<DaemonDbusInterface>(uri, 1, 0, "DaemonDbusInterface",
        [](QQmlEngine*, QJSEngine*) -> QObject* {
            return new DaemonDbusInterface;
        }
    );
}

void KdeConnectDeclarativePlugin::initializeEngine(QQmlEngine* engine, const char* uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);
 
    engine->rootContext()->setContextProperty(QStringLiteral("DeviceDbusInterfaceFactory")
      , new ObjectFactory(engine, createDeviceDbusInterface));
    
    engine->rootContext()->setContextProperty(QStringLiteral("DeviceBatteryDbusInterfaceFactory")
      , new ObjectFactory(engine, createDeviceBatteryDbusInterface));
    
    engine->rootContext()->setContextProperty(QStringLiteral("FindMyPhoneDbusInterfaceFactory")
      , new ObjectFactory(engine, createFindMyPhoneInterface));

    engine->rootContext()->setContextProperty(QStringLiteral("SftpDbusInterfaceFactory")
      , new ObjectFactory(engine, createSftpInterface));

    engine->rootContext()->setContextProperty(QStringLiteral("RemoteKeyboardDbusInterfaceFactory")
      , new ObjectFactory(engine, createRemoteKeyboardInterface));

    engine->rootContext()->setContextProperty(QStringLiteral("MprisDbusInterfaceFactory")
      , new ObjectFactory(engine, createMprisInterface));

    engine->rootContext()->setContextProperty(QStringLiteral("RemoteControlDbusInterfaceFactory")
      , new ObjectFactory(engine, createRemoteControlInterface));

    engine->rootContext()->setContextProperty(QStringLiteral("LockDeviceDbusInterfaceFactory")
      , new ObjectFactory(engine, createDeviceLockInterface));
    
    engine->rootContext()->setContextProperty(QStringLiteral("DBusResponseFactory")
      , new ObjectFactory(engine, createDBusResponse));    
    
    engine->rootContext()->setContextProperty(QStringLiteral("DBusResponseWaiter")
      , DBusResponseWaiter::instance());
}
