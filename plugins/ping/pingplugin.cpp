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

#include "pingplugin.h"

#include <KNotification>
#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QLoggingCategory>

#include <core/device.h>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_ping.json", registerPlugin< PingPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_PING, "kdeconnect.plugin.ping")

PingPlugin::PingPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
//     qCDebug(KDECONNECT_PLUGIN_PING) << "Ping plugin constructor for device" << device()->name();
}

PingPlugin::~PingPlugin()
{
//     qCDebug(KDECONNECT_PLUGIN_PING) << "Ping plugin destructor for device" << device()->name();
}

bool PingPlugin::receivePackage(const NetworkPackage& np)
{
    KNotification* notification = new KNotification(QStringLiteral("pingReceived")); //KNotification::Persistent
    notification->setIconName(QStringLiteral("dialog-ok"));
    notification->setComponentName(QStringLiteral("kdeconnect"));
    notification->setTitle(device()->name());
    notification->setText(np.get<QString>(QStringLiteral("message"),i18n("Ping!"))); //This can be a source of spam
    notification->sendEvent();

    return true;
}

void PingPlugin::sendPing()
{
    NetworkPackage np(PACKAGE_TYPE_PING);
    bool success = sendPackage(np);
    qCDebug(KDECONNECT_PLUGIN_PING) << "sendPing:" << success;
}

void PingPlugin::sendPing(const QString& customMessage)
{
    NetworkPackage np(PACKAGE_TYPE_PING);
    if (!customMessage.isEmpty()) {
        np.set(QStringLiteral("message"), customMessage);
    }
    bool success = sendPackage(np);
    qCDebug(KDECONNECT_PLUGIN_PING) << "sendPing:" << success;
}

QString PingPlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/ping";
}

#include "pingplugin.moc"

