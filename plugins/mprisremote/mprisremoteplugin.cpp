/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mprisremoteplugin.h"

#include <KPluginFactory>

#include <QDebug>
#include <QStandardPaths>

#include <core/device.h>
#include <filetransferjob.h>

#include "albumart_cache.h"
#include "plugin_mprisremote_debug.h"

K_PLUGIN_CLASS_WITH_JSON(MprisRemotePlugin, "kdeconnect_mprisremote.json")

void MprisRemotePlugin::receivePacket(const NetworkPacket &np)
{
    if (np.type() != PACKET_TYPE_MPRIS)
        return;

    if (np.get<bool>(QStringLiteral("transferringAlbumArt"), false)) {
        AlbumArtCache::instance()->handleAlbumArt(np);
        return;
    }

    if (np.has(QStringLiteral("player"))) {
        const QString player = np.get<QString>(QStringLiteral("player"));
        if (!m_players.contains(player)) {
            m_players[player] = new MprisRemotePlayer(player, this);
        }
        m_players[player]->parseNetworkPacket(np);
    }

    if (np.has(QStringLiteral("playerList"))) {
        QStringList players = np.get<QStringList>(QStringLiteral("playerList"));

        // Remove players not available any more
        for (auto iter = m_players.begin(); iter != m_players.end();) {
            if (!players.contains(iter.key())) {
                iter.value()->deleteLater();
                iter = m_players.erase(iter);
            } else {
                ++iter;
            }
        }

        // Add new players
        for (const QString &player : players) {
            if (!m_players.contains(player)) {
                m_players[player] = new MprisRemotePlayer(player, this);
                requestPlayerStatus(player);
            }
        }

        if (m_players.empty()) {
            m_currentPlayer = QString();
        } else if (!m_players.contains(m_currentPlayer)) {
            m_currentPlayer = m_players.firstKey();
        }
    }
    Q_EMIT propertiesChanged();
}

long MprisRemotePlugin::position() const
{
    auto player = m_players.value(m_currentPlayer);
    return player ? player->position() : 0;
}

QString MprisRemotePlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/mprisremote").arg(device()->id());
}

void MprisRemotePlugin::requestPlayerStatus(const QString &player)
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST,
                     {{QStringLiteral("player"), player}, {QStringLiteral("requestNowPlaying"), true}, {QStringLiteral("requestVolume"), true}});
    sendPacket(np);
}

void MprisRemotePlugin::requestPlayerList()
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {{QStringLiteral("requestPlayerList"), true}});
    sendPacket(np);
}

void MprisRemotePlugin::requestAlbumArt(const QString &player, const QString &album_art_url)
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {{QStringLiteral("player"), player}, {QStringLiteral("albumArtUrl"), album_art_url}});
    qInfo(KDECONNECT_PLUGIN_MPRISREMOTE) << "Requesting album art " << np.serialize();
    sendPacket(np);
}

void MprisRemotePlugin::sendAction(const QString &action)
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {{QStringLiteral("player"), m_currentPlayer}, {QStringLiteral("action"), action}});
    sendPacket(np);
}

void MprisRemotePlugin::seek(int offset) const
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {{QStringLiteral("player"), m_currentPlayer}, {QStringLiteral("Seek"), offset}});
    sendPacket(np);
}

void MprisRemotePlugin::setVolume(int volume)
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {{QStringLiteral("player"), m_currentPlayer}, {QStringLiteral("setVolume"), volume}});
    sendPacket(np);
}

void MprisRemotePlugin::setPosition(int position)
{
    NetworkPacket np(PACKET_TYPE_MPRIS_REQUEST, {{QStringLiteral("player"), m_currentPlayer}, {QStringLiteral("SetPosition"), position}});
    sendPacket(np);

    m_players[m_currentPlayer]->setPosition(position);
}

void MprisRemotePlugin::setPlayer(const QString &player)
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

bool MprisRemotePlugin::canSeek() const
{
    auto player = m_players.value(m_currentPlayer);
    return player ? player->canSeek() : false;
}

#include "moc_mprisremoteplugin.cpp"
#include "mprisremoteplugin.moc"
