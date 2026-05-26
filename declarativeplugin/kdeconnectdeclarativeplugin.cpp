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

QObject *createDBusResponse()
{
    return new DBusAsyncResponse();
}

void KdeConnectDeclarativePlugin::registerTypes(const char * /*uri*/)
{
}

void KdeConnectDeclarativePlugin::initializeEngine(QQmlEngine *engine, const char *uri)
{
    QQmlExtensionPlugin::initializeEngine(engine, uri);

    engine->rootContext()->setContextProperty(QStringLiteral("DBusResponseFactory"), new ObjectFactory(engine, createDBusResponse));

    engine->rootContext()->setContextProperty(QStringLiteral("DBusResponseWaiter"), DBusResponseWaiter::instance());
}

#include "moc_kdeconnectdeclarativeplugin.cpp"
