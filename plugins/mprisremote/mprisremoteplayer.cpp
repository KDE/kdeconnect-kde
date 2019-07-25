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

#include "mprisremoteplayer.h"
#include "mprisremoteplugin.h"
#include "mprisremoteplayermediaplayer2.h"
#include "mprisremoteplayermediaplayer2player.h"

#include <QDateTime>

#include <networkpacket.h>
#include <QUuid>

MprisRemotePlayer::MprisRemotePlayer(QString id, MprisRemotePlugin *plugin) :
    QObject(plugin)
    , id(id)
    , m_playing(false)
    , m_canPlay(true)
    , m_canPause(true)
    , m_canGoPrevious(true)
    , m_canGoNext(true)
    , m_nowPlaying()
    , m_volume(50)
    , m_length(0)
    , m_lastPosition(0)
    , m_lastPositionTime()
    , m_title()
    , m_artist()
    , m_album()
    , m_canSeek(false)
    , m_dbusConnectionName(QStringLiteral("mpris_") + QUuid::createUuid().toString(QUuid::Id128))
    , m_dbusConnection(QDBusConnection::connectToBus(QDBusConnection::SessionBus, m_dbusConnectionName))
{
    //Expose this player on the newly created connection. This allows multiple mpris services in the same Qt process
    new MprisRemotePlayerMediaPlayer2(this, plugin);
    new MprisRemotePlayerMediaPlayer2Player(this, plugin);

    m_dbusConnection.registerObject(QStringLiteral("/org/mpris/MediaPlayer2"), this);
    //Make sure our service name is unique. Reuse the connection name for this.
    m_dbusConnection.registerService(QStringLiteral("org.mpris.MediaPlayer2.kdeconnect.") + m_dbusConnectionName);
}

MprisRemotePlayer::~MprisRemotePlayer()
{
    //Drop the DBus connection (it was only used for this class)
    QDBusConnection::disconnectFromBus(m_dbusConnectionName);
}

void MprisRemotePlayer::parseNetworkPacket(const NetworkPacket& np)
{
    bool trackInfoHasChanged = false;

    //Track properties
    QString newNowPlaying = np.get<QString>(QStringLiteral("nowPlaying"), m_nowPlaying);
    QString newTitle = np.get<QString>(QStringLiteral("title"), m_title);
    QString newArtist = np.get<QString>(QStringLiteral("artist"), m_artist);
    QString newAlbum = np.get<QString>(QStringLiteral("album"), m_album);
    int newLength = np.get<int>(QStringLiteral("length"), m_length);

    //Check if they changed
    if (newNowPlaying != m_nowPlaying || newTitle != m_title || newArtist != m_artist || newAlbum != m_album || newLength != m_length) {
        trackInfoHasChanged = true;
        Q_EMIT trackInfoChanged();
    }
    //Set the new values
    m_nowPlaying = newNowPlaying;
    m_title = newTitle;
    m_artist = newArtist;
    m_album = newAlbum;
    m_length = newLength;

    //Check volume changes
    int newVolume = np.get<int>(QStringLiteral("volume"), m_volume);
    if (newVolume != m_volume) {
        Q_EMIT volumeChanged();
    }
    m_volume = newVolume;

    if (np.has(QStringLiteral("pos"))) {
        //Check position
        int newLastPosition = np.get<int>(QStringLiteral("pos"), m_lastPosition);
        int positionDiff = qAbs(position() - newLastPosition);
        m_lastPosition = newLastPosition;
        m_lastPositionTime = QDateTime::currentMSecsSinceEpoch();

        //Only consider it seeking if the position changed more than 1 second, and the track has not changed
        if (qAbs(positionDiff) >= 1000 && !trackInfoHasChanged) {
            Q_EMIT positionChanged();
        }
    }

    //Check if we started/stopped playing
    bool newPlaying = np.get<bool>(QStringLiteral("isPlaying"), m_playing);
    if (newPlaying != m_playing) {
        Q_EMIT playingChanged();
    }
    m_playing = newPlaying;

    //Control properties
    bool newCanSeek = np.get<bool>(QStringLiteral("canSeek"), m_canSeek);
    bool newCanPlay = np.get<bool>(QStringLiteral("canPlay"), m_canPlay);
    bool newCanPause = np.get<bool>(QStringLiteral("canPause"), m_canPause);
    bool newCanGoPrevious = np.get<bool>(QStringLiteral("canGoPrevious"), m_canGoPrevious);
    bool newCanGoNext = np.get<bool>(QStringLiteral("canGoNext"), m_canGoNext);

    //Check if they changed
    if (newCanSeek != m_canSeek || newCanPlay != m_canPlay || newCanPause != m_canPause || newCanGoPrevious != m_canGoPrevious || newCanGoNext != m_canGoNext) {
        Q_EMIT controlsChanged();
    }
    //Set the new values
    m_canSeek = newCanSeek;
    m_canPlay = newCanPlay;
    m_canPause = newCanPause;
    m_canGoPrevious = newCanGoPrevious;
    m_canGoNext = newCanGoNext;
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

bool MprisRemotePlayer::canSeek() const
{
    return m_canSeek;
}

QString MprisRemotePlayer::identity() const {
    return id;
}

bool MprisRemotePlayer::canPlay() const {
    return m_canPlay;
}

bool MprisRemotePlayer::canPause() const {
    return m_canPause;
}

bool MprisRemotePlayer::canGoPrevious() const {
    return m_canGoPrevious;
}

bool MprisRemotePlayer::canGoNext() const {
    return m_canGoNext;
}

QDBusConnection & MprisRemotePlayer::dbus() {
    return m_dbusConnection;
}
