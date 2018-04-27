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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <networkpacket.h>
#include <QDateTime>

#include "mprisremoteplayer.h"

MprisRemotePlayer::MprisRemotePlayer() :
    m_playing(false)
    , m_nowPlaying()
    , m_volume(50)
    , m_length(0)
    , m_lastPosition(0)
    , m_lastPositionTime()
    , m_title()
    , m_artist()
    , m_album()
{
}

MprisRemotePlayer::~MprisRemotePlayer()
{
}

void MprisRemotePlayer::parseNetworkPacket(const NetworkPacket& np)
{
    m_nowPlaying = np.get<QString>(QStringLiteral("nowPlaying"), m_nowPlaying);
    m_title = np.get<QString>(QStringLiteral("title"), m_title);
    m_artist = np.get<QString>(QStringLiteral("artist"), m_artist);
    m_album = np.get<QString>(QStringLiteral("album"), m_album);
    m_volume = np.get<int>(QStringLiteral("volume"), m_volume);
    m_length = np.get<int>(QStringLiteral("length"), m_length);
    if(np.has(QStringLiteral("pos"))){
        m_lastPosition = np.get<int>(QStringLiteral("pos"), m_lastPosition);
        m_lastPositionTime = QDateTime::currentMSecsSinceEpoch();
    }
    m_playing = np.get<bool>(QStringLiteral("isPlaying"), m_playing);
}

long MprisRemotePlayer::position() const
{
    if(m_playing) {
        return m_lastPosition + (QDateTime::currentMSecsSinceEpoch() - m_lastPositionTime);
    } else {
        return m_lastPosition;
    }
}

void MprisRemotePlayer::setPosition(long position)
{
    m_lastPosition = position;
    m_lastPositionTime = QDateTime::currentMSecsSinceEpoch();
}

int MprisRemotePlayer::volume() const
{
    return m_volume;
}

long int MprisRemotePlayer::length() const
{
    return m_length;
}

bool MprisRemotePlayer::playing() const
{
    return m_playing;
}

QString MprisRemotePlayer::nowPlaying() const
{
    return m_nowPlaying;
}

QString MprisRemotePlayer::title() const
{
    return m_title;
}

QString MprisRemotePlayer::artist() const
{
    return m_artist;
}

QString MprisRemotePlayer::album() const
{
    return m_album;
}
