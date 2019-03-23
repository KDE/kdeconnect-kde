/**
 * Copyright 2018 Nicolas Fella <nicolas.fella@gmx.de>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "remotesystemvolumeplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QLoggingCategory>
#include <QJsonArray>
#include <QJsonDocument>

#include <core/device.h>
#include <core/daemon.h>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_remotesystemvolume.json", registerPlugin< RemoteSystemVolumePlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_PING, "kdeconnect.plugin.remotesystemvolume")

RemoteSystemVolumePlugin::RemoteSystemVolumePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

RemoteSystemVolumePlugin::~RemoteSystemVolumePlugin()
{
}

bool RemoteSystemVolumePlugin::receivePacket(const NetworkPacket& np)
{

    if (np.has(QStringLiteral("sinkList"))) {
        QJsonDocument document(np.get<QJsonArray>(QStringLiteral("sinkList")));
        m_sinks = document.toJson();
        Q_EMIT sinksChanged();
    } else {

        QString name = np.get<QString>(QStringLiteral("name"));

        if (np.has(QStringLiteral("volume"))) {
            Q_EMIT volumeChanged(name, np.get<int>(QStringLiteral("volume")));
        }

        if (np.has(QStringLiteral("muted"))) {
            Q_EMIT mutedChanged(name, np.get<int>(QStringLiteral("muted")));
        }
    }

    return true;
}

void RemoteSystemVolumePlugin::sendVolume(const QString& name, int volume)
{
    NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME_REQUEST);
    np.set<QString>(QStringLiteral("name"), name);
    np.set<int>(QStringLiteral("volume"), volume);
    sendPacket(np);
}

void RemoteSystemVolumePlugin::sendMuted(const QString& name, bool muted)
{
    NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME_REQUEST);
    np.set<QString>(QStringLiteral("name"), name);
    np.set<bool>(QStringLiteral("muted"), muted);
    sendPacket(np);
}

void RemoteSystemVolumePlugin::connected()
{
    NetworkPacket np(PACKET_TYPE_SYSTEMVOLUME_REQUEST);
    np.set<bool>(QStringLiteral("requestSinks"), true);
    sendPacket(np);
}

QByteArray RemoteSystemVolumePlugin::sinks()
{
    return m_sinks;
}

QString RemoteSystemVolumePlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/remotesystemvolume";
}

#include "remotesystemvolumeplugin.moc"

