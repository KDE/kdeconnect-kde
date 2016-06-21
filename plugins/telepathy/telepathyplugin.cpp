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

#include "telepathyplugin.h"

#include <KLocalizedString>
#include <QIcon>
#include <QDebug>

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_telepathy.json", registerPlugin< TelepathyPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_TELEPHONY, "kdeconnect.plugin.telephony")

TelepathyPlugin::TelepathyPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_telepathyInterface(new OrgFreedesktopTelepathyConnectionManagerKdeconnectInterface("org.freedesktop.Telepathy.ConnectionManager.kdeconnect", "/kdeconnect", QDBusConnection::sessionBus(), this))
{
    connect(m_telepathyInterface, SIGNAL(messageReceived(QString,QString)), SLOT(sendSms(QString,QString)));
}

bool TelepathyPlugin::receivePackage(const NetworkPackage& np)
{
    if (np.get<QString>("event") == QLatin1String("sms")) {
        const QString messageBody = np.get<QString>("messageBody","");
        const QString phoneNumber = np.get<QString>("phoneNumber", i18n("unknown number"));
        const QString contactName = np.get<QString>("contactName", phoneNumber);
        if (m_telepathyInterface->sendMessage(contactName, messageBody)) {
             return true;
        }
    }

    return true;
}

void TelepathyPlugin::sendSms(const QString& phoneNumber, const QString& messageBody)
{
    NetworkPackage np(PACKAGE_TYPE_SMS_REQUEST, {
        {"sendSms", true},
        {"phoneNumber", phoneNumber},
        {"messageBody", messageBody}
    });
    sendPackage(np);
}

#include "telepathyplugin.moc"
