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

#include "mprisremoteplugin.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <QDebug>
#include <QDBusConnection>
#include <QLoggingCategory>

#include <core/device.h>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_mprisremote.json", registerPlugin< MprisRemotePlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_MPRISREMOTE, "kdeconnect.plugin.mprisremote")

MprisRemotePlugin::MprisRemotePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , m_player()
    , m_playing(false)
    , m_nowPlaying()
    , m_volume(50)
    , m_length(0)
    , m_lastPosition(0)
    , m_lastPositionTime()
    , m_playerList()
{
}

MprisRemotePlugin::~MprisRemotePlugin()
{
}

bool MprisRemotePlugin::receivePackage(const NetworkPackage& np)
{
    if (np.type() != PACKAGE_TYPE_MPRIS)
        return false;

    if (np.has(QStringLiteral("nowPlaying")) || np.has(QStringLiteral("volume")) || np.has(QStringLiteral("isPlaying")) || np.has(QStringLiteral("length")) || np.has(QStringLiteral("pos"))) {
        if (np.get<QString>(QStringLiteral("player")) == m_player) {
            m_nowPlaying = np.get<QString>(QStringLiteral("nowPlaying"), m_nowPlaying);
            m_volume = np.get<int>(QStringLiteral("volume"), m_volume);
            m_length = np.get<int>(QStringLiteral("length"), m_length);
            if(np.has(QStringLiteral("pos"))){
                m_lastPosition = np.get<int>(QStringLiteral("pos"), m_lastPosition);
                m_lastPositionTime = QDateTime::currentMSecsSinceEpoch();
            }
            m_playing = np.get<bool>(QStringLiteral("isPlaying"), m_playing);
        }
    }

    if (np.has(QStringLiteral("playerList"))) {
        m_playerList = np.get<QStringList>(QStringLiteral("playerList"), QStringList());
    }
    Q_EMIT propertiesChanged();

    return true;
}

long MprisRemotePlugin::position() const
{
    if(m_playing) {
        return m_lastPosition + (QDateTime::currentMSecsSinceEpoch() - m_lastPositionTime);
    } else {
        return m_lastPosition;
    }
}

QString MprisRemotePlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/mprisremote";
}

void MprisRemotePlugin::requestPlayerStatus()
{
    NetworkPackage np(PACKAGE_TYPE_MPRIS_REQUEST, {
        {"player", m_player},
        {"requestNowPlaying", true},
        {"requestVolume", true}}
    );
    sendPackage(np);
}

void MprisRemotePlugin::requestPlayerList()
{
    NetworkPackage np(PACKAGE_TYPE_MPRIS_REQUEST, {{"requestPlayerList", true}});
    sendPackage(np);
}

void MprisRemotePlugin::sendAction(const QString& action)
{
    NetworkPackage np(PACKAGE_TYPE_MPRIS_REQUEST, {
        {"player", m_player},
        {"action", action}
    });
    sendPackage(np);
}

void MprisRemotePlugin::seek(int offset) const
{
    NetworkPackage np(PACKAGE_TYPE_MPRIS_REQUEST, {
        {"player", m_player},
        {"Seek", offset}});
    sendPackage(np);
}

void MprisRemotePlugin::setVolume(int volume)
{
    NetworkPackage np(PACKAGE_TYPE_MPRIS_REQUEST, {
        {"player", m_player},
        {"setVolume",volume}
    });
    sendPackage(np);
}

void MprisRemotePlugin::setPosition(int position)
{
    NetworkPackage np(PACKAGE_TYPE_MPRIS_REQUEST, {
        {"player", m_player},
        {"SetPosition", position}
    });
    sendPackage(np);

    m_lastPosition = position;
    m_lastPositionTime = QDateTime::currentMSecsSinceEpoch();
}

void MprisRemotePlugin::setPlayer(const QString& player)
{
    if (m_player != player) {
        m_player = player;
        requestPlayerStatus();
    }
}

#include "mprisremoteplugin.moc"
