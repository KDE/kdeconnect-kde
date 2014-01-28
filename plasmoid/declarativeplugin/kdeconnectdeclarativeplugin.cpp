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

#include "libkdeconnect/devicesmodel.h"
#include "libkdeconnect/notificationsmodel.h"
#include "batteryinterface.h"

Q_EXPORT_PLUGIN2(kdeconnectdeclarativeplugin, KdeConnectDeclarativePlugin);

// Q_DECLARE_METATYPE(QDBusPendingCall)

Q_DECLARE_METATYPE(QDBusPendingReply<>)
Q_DECLARE_METATYPE(QDBusPendingReply<QVariant>)
Q_DECLARE_METATYPE(QDBusPendingReply<bool>)
Q_DECLARE_METATYPE(QDBusPendingReply<QString>)

QObject* createSftpInterface(QVariant deviceId)
{
    return new SftpDbusInterface(deviceId.toString());
}

const QDBusPendingCall* extractPendingCall(QVariant& variant)
{
    if (variant.canConvert<QDBusPendingReply<> >())
    {}
    else if (variant.canConvert<QDBusPendingReply<QVariant> >())
    {}
    else if (variant.canConvert<QDBusPendingReply<bool> >())
    {}
    else if (variant.canConvert<QDBusPendingReply<QString> >())
    {}
    else
    {
        return 0;
    }
    
    return reinterpret_cast<const QDBusPendingCall*>(variant.constData());
}

QVariant DBusResponseWaiter::waitForReply(QVariant variant) const
{
    if (QDBusPendingCall* call = const_cast<QDBusPendingCall*>(extractPendingCall(variant)))
    {
        call->waitForFinished();
        QDBusMessage reply = call->reply();

        if (reply.arguments().count() > 0)
        {
            qDebug() <<reply.arguments().first(); 
            return reply.arguments().first();
        }
        else
        {
            return QVariant();
        }
    }
    return QVariant();
}

void KdeConnectDeclarativePlugin::registerTypes(const char* uri)
{
    Q_UNUSED(uri);
    
    qRegisterMetaType<QDBusPendingReply<> >("QDBusPendingReply<>");
    qRegisterMetaType<QDBusPendingReply<QVariant> >("QDBusPendingReply<QVariant>");
    qRegisterMetaType<QDBusPendingReply<bool> >("QDBusPendingReply<bool>");
    qRegisterMetaType<QDBusPendingReply<QString> >("QDBusPendingReply<QString>");

    qmlRegisterType<DevicesModel>("org.kde.kdeconnect", 1, 0, "DevicesModel");
    qmlRegisterType<NotificationsModel>("org.kde.kdeconnect", 1, 0, "NotificationsModel");
    qmlRegisterType<BatteryInterface>("org.kde.kdeconnect", 1, 0, "BatteryInterface");
}

void KdeConnectDeclarativePlugin::initializeEngine(QDeclarativeEngine* engine, const char* uri)
{
    QDeclarativeExtensionPlugin::initializeEngine(engine, uri);
    
    engine->rootContext()->setContextProperty("SftpDbusInterfaceFactory"
      , new ObjectFactory(engine, createSftpInterface));
    
    engine->rootContext()->setContextProperty("ResponseWaiter"
      , new DBusResponseWaiter());    
}
