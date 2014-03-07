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

#include <QtDeclarative/QDeclarativeItem>
#include <QtDeclarative/QDeclarativeEngine>
#include <QtDeclarative/QDeclarativeContext>
#include <QDBusPendingCall>
#include <QDBusPendingReply>

#include "objectfactory.h"
#include "responsewaiter.h"

#include "libkdeconnect/devicesmodel.h"
#include "libkdeconnect/notificationsmodel.h"

Q_EXPORT_PLUGIN2(kdeconnectdeclarativeplugin, KdeConnectDeclarativePlugin)

QObject* createDeviceDbusInterface(QVariant deviceId)
{
    return new DeviceDbusInterface(deviceId.toString());
}

QObject* createDeviceBatteryDbusInterface(QVariant deviceId)
{
    return new DeviceBatteryDbusInterface(deviceId.toString());
}

QObject* createSftpInterface(QVariant deviceId)
{
    return new SftpDbusInterface(deviceId.toString());
}

QObject* createDBusResponse()
{
    return new DBusAsyncResponse();
}

void KdeConnectDeclarativePlugin::registerTypes(const char* uri)
{
    Q_UNUSED(uri);
    
    qmlRegisterType<DevicesModel>("org.kde.kdeconnect", 1, 0, "DevicesModel");
    qmlRegisterType<NotificationsModel>("org.kde.kdeconnect", 1, 0, "NotificationsModel");
    qmlRegisterType<DBusAsyncResponse>("org.kde.kdeconnect", 1, 0, "DBusAsyncResponse");
}

void KdeConnectDeclarativePlugin::initializeEngine(QDeclarativeEngine* engine, const char* uri)
{
    QDeclarativeExtensionPlugin::initializeEngine(engine, uri);
 
    engine->rootContext()->setContextProperty("DeviceDbusInterfaceFactory"
      , new ObjectFactory(engine, createDeviceDbusInterface));
    
    engine->rootContext()->setContextProperty("DeviceBatteryDbusInterfaceFactory"
      , new ObjectFactory(engine, createDeviceBatteryDbusInterface));
    
    engine->rootContext()->setContextProperty("SftpDbusInterfaceFactory"
      , new ObjectFactory(engine, createSftpInterface));
    
    engine->rootContext()->setContextProperty("DBusResponseFactory"
      , new ObjectFactory(engine, createDBusResponse));    
    
    engine->rootContext()->setContextProperty("DBusResponseWaiter"
      , DBusResponseWaiter::instance());    
}
