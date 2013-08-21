/*
 *   Copyright (C) 2010 Jacopo De Simoi <wilderkde@gmail.com>
 *
 * This program is free software you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
*/

#include "kdeconnectengine.h"

#include <QDBusConnection>

#include <Plasma/DataContainer>

#include "plasmoidadaptor.h"

#include <KDebug>

KdeConnectEngine::KdeConnectEngine( QObject* parent, const QVariantList& args )
    : Plasma::DataEngine( parent, args ),
      m_id(0)
{
    new PlasmoidAdaptor(this);

    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.registerService( "org.kde.kdeconnect.plasmoid" );
    dbus.registerObject( "/modules/kdeconnect/plasmoid", this );
}

KdeConnectEngine::~KdeConnectEngine()
{
    QDBusConnection dbus = QDBusConnection::sessionBus();
    dbus.unregisterService( "org.kde.kdeconnect.plasmoid" );
}

void KdeConnectEngine::init()
{
}

void KdeConnectEngine::notify(int solidError, const QString& error, const QString& errorDetails, const QString &udi)
{
    kDebug() << error << errorDetails << udi;
    const QString source = QString("notification %1").arg(m_id++);

    Plasma::DataEngine::Data notificationData;
    notificationData.insert("solidError", solidError);
    notificationData.insert("error", error);
    notificationData.insert("errorDetails", errorDetails);
    notificationData.insert("udi", udi);

    setData(source, notificationData );
}

K_EXPORT_PLASMA_DATAENGINE(devicenotifications, KdeConnectEngine)
