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
    , m_currentPlayer()
    , m_players()
{
}

MprisRemotePlugin::~MprisRemotePlugin()
{
}

bool MprisRemotePlugin::receivePacket(const NetworkPacket& np)
{
    if (np.type() != PACKET_TYPE_MPRIS)
        return false;

    if (np.has(QStringLiteral("player"))) {
        m_players[np.get<QString>(QStringLiteral("player"))]->parseNetworkPacket(np);
    }

    if (np.has(QStringLiteral("playerList"))) {
        QStringList players = np.get<QStringList>(QStringLiteral("playerList"));
        qDeleteAll(m_players);
        m_players.clear();
        for (const QString& player : players) {
            m_players[player] = new MprisRemotePlayer();
            requestPlayerStatus(player);
        }

        if (m_players.empty()) {
            m_currentPlayer = QString();
        } else if (!m_players.contains(m_currentPlayer)) {
            m_currentPlayer = m_players.keys().first();
        }

    }
    Q_EMIT propertiesChanged();

    return true;
}

long MprisRemotePlugin::position() const
{
    auto player = m_players.value(m_currentPlayer);
    return player ? player->position() : 0;
}

QString MprisRemotePlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/mprisremote";
}

void MprisRemotePlugin::requestPlayerStatus(const QString& player)
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {
        {"player", player},
        {"requestNowPlaying", true},
        {"requestVolume", true}}
    );
    sendPacket(np);
}

void MprisRemotePlugin::requestPlayerList()
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {{"requestPlayerList", true}});
    sendPacket(np);
}

void MprisRemotePlugin::sendAction(const QString& action)
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {
        {"player", m_currentPlayer},
        {"action", action}
    });
    sendPacket(np);
}

void MprisRemotePlugin::seek(int offset) const
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {
        {"player", m_currentPlayer},
        {"Seek", offset}});
    sendPacket(np);
}

void MprisRemotePlugin::setVolume(int volume)
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {
        {"player", m_currentPlayer},
        {"setVolume",volume}
    });
    sendPacket(np);
}

void MprisRemotePlugin::setPosition(int position)
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {
        {"player", m_currentPlayer},
        {"SetPosition", position}
    });
    sendPacket(np);

    m_players[m_currentPlayer]->setPosition(position);
}

void MprisRemotePlugin::setPlayer(const QString& player)
{
    if (m_currentPlayer != player) {
        m_currentPlayer = player;
        requestPlayerStatus(player);
        Q_EMIT propertiesChanged();
    }
}

bool MprisRemotePlugin::isPlaying() const
{
    auto player = m_players.value(m_currentPlayer);
    return player ? player->playing() : false;
}

int MprisRemotePlugin::length() const
{
    auto player = m_players.value(m_currentPlayer);
    return player ? player->length() : 0;
}

int MprisRemotePlugin::volume() const
{
    auto player = m_players.value(m_currentPlayer);
    return player ? player->volume() : 0;
}

QString MprisRemotePlugin::player() const
{
    if (m_currentPlayer.isEmpty())
        return QString();
    return m_currentPlayer;
}

QStringList MprisRemotePlugin::playerList() const
{
    return m_players.keys();
}

QString MprisRemotePlugin::nowPlaying() const
{
    auto player = m_players.value(m_currentPlayer);
    return player ? player->nowPlaying() : QString();
}

QString MprisRemotePlugin::title() const
{
    auto player = m_players.value(m_currentPlayer);
    return player ? player->title() : QString();
}

QString MprisRemotePlugin::album() const
{
    auto player = m_players.value(m_currentPlayer);
    return player ? player->album() : QString();
}

QString MprisRemotePlugin::artist() const
{
    auto player = m_players.value(m_currentPlayer);
    return player ? player->artist() : QString();
}

#include "mprisremoteplugin.moc"
