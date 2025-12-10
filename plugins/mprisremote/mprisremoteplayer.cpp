/**
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mprisremoteplayer.h"
#include "albumart_cache.h"
#include "mprisremoteplayermediaplayer2.h"
#include "mprisremoteplayermediaplayer2player.h"
#include "mprisremoteplugin.h"

#include <QDateTime>

#include <QUuid>
#include <networkpacket.h>

#include "plugin_mprisremote_debug.h"

MprisRemotePlayer::MprisRemotePlayer(QString id, MprisRemotePlugin *plugin)
    : QObject(plugin)
    , id(id)
    , m_playing(false)
    , m_canPlay(true)
    , m_canPause(true)
    , m_canGoPrevious(true)
    , m_canGoNext(true)
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
    connect(AlbumArtCache::instance(), &AlbumArtCache::albumArtFetched, this, &MprisRemotePlayer::albumArtFetched);
    // Expose this player on the newly created connection. This allows multiple mpris services in the same Qt process
    new MprisRemotePlayerMediaPlayer2(this, plugin);
    new MprisRemotePlayerMediaPlayer2Player(this, plugin);

    m_dbusConnection.registerObject(QStringLiteral("/org/mpris/MediaPlayer2"), this);
    // Make sure our service name is unique. Reuse the connection name for this.
    m_dbusConnection.registerService(QStringLiteral("org.mpris.MediaPlayer2.kdeconnect.") + m_dbusConnectionName);
}

MprisRemotePlayer::~MprisRemotePlayer()
{
    // Drop the DBus connection (it was only used for this class)
    QDBusConnection::disconnectFromBus(m_dbusConnectionName);
}

void MprisRemotePlayer::parseNetworkPacket(const NetworkPacket &np)
{
    bool trackInfoHasChanged = false;

    // Track properties
    QString newTitle = np.get<QString>(QStringLiteral("title"), m_title);
    QString newArtist = np.get<QString>(QStringLiteral("artist"), m_artist);
    QString newAlbum = np.get<QString>(QStringLiteral("album"), m_album);
    QString newAlbumArtUrl = np.get<QString>(QStringLiteral("albumArtUrl"), m_albumArtUrl);
    int newLength = np.get<int>(QStringLiteral("length"), m_length);
    if (newAlbumArtUrl != m_albumArtUrl) {
        // album art changed
        m_albumArtUrl = newAlbumArtUrl;
        auto url = AlbumArtCache::getAlbumArt(newAlbumArtUrl, (MprisRemotePlugin *)(parent()), identity());
        // if empty, will be populated once album art has been fetched
        if (url.fetchStatus != AlbumArtCache::IndexItem::FETCHING) {
            setLocalAlbumArtUrl(url.file);
        }
    }

    // Check if they changed
    if (newTitle != m_title || newArtist != m_artist || newAlbum != m_album || newLength != m_length) {
        trackInfoHasChanged = true;
        m_title = newTitle;
        m_artist = newArtist;
        m_album = newAlbum;
        m_length = newLength;
        Q_EMIT trackInfoChanged();
    }

    // Check volume changes
    int newVolume = np.get<int>(QStringLiteral("volume"), m_volume);
    if (newVolume != m_volume) {
        m_volume = newVolume;
        Q_EMIT volumeChanged();
    }

    if (np.has(QStringLiteral("pos"))) {
        // Check position
        int newLastPosition = np.get<int>(QStringLiteral("pos"), m_lastPosition);
        int positionDiff = qAbs(position() - newLastPosition);
        m_lastPosition = newLastPosition;
        m_lastPositionTime = QDateTime::currentMSecsSinceEpoch();

        // Only consider it seeking if the position changed more than 1 second or the track has changed
        if (qAbs(positionDiff) >= 1000 || trackInfoHasChanged) {
            Q_EMIT positionChanged();
        }
    }

    // Check if we started/stopped playing
    bool newPlaying = np.get<bool>(QStringLiteral("isPlaying"), m_playing);
    if (newPlaying != m_playing) {
        m_playing = newPlaying;
        Q_EMIT playingChanged();
    }

    // Control properties
    bool newCanSeek = np.get<bool>(QStringLiteral("canSeek"), m_canSeek);
    bool newCanPlay = np.get<bool>(QStringLiteral("canPlay"), m_canPlay);
    bool newCanPause = np.get<bool>(QStringLiteral("canPause"), m_canPause);
    bool newCanGoPrevious = np.get<bool>(QStringLiteral("canGoPrevious"), m_canGoPrevious);
    bool newCanGoNext = np.get<bool>(QStringLiteral("canGoNext"), m_canGoNext);
    if (newCanSeek != m_canSeek || newCanPlay != m_canPlay || newCanPause != m_canPause || newCanGoPrevious != m_canGoPrevious || newCanGoNext != m_canGoNext) {
        m_canSeek = newCanSeek;
        m_canPlay = newCanPlay;
        m_canPause = newCanPause;
        m_canGoPrevious = newCanGoPrevious;
        m_canGoNext = newCanGoNext;
        Q_EMIT controlsChanged();
    }
}

long MprisRemotePlayer::position() const
{
    if (m_playing) {
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

void MprisRemotePlayer::setLocalAlbumArtUrl(const QSharedPointer<LocalFile> &url)
{
    m_localAlbumArtUrl = url;
    qCDebug(KDECONNECT_PLUGIN_MPRISREMOTE) << "Setting local url" << (url ? url->localPath.toDisplayString() : QStringLiteral("(null)"));
    Q_EMIT trackInfoChanged();
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

QString MprisRemotePlayer::identity() const
{
    return id;
}

bool MprisRemotePlayer::canPlay() const
{
    return m_canPlay;
}

bool MprisRemotePlayer::canPause() const
{
    return m_canPause;
}

bool MprisRemotePlayer::canGoPrevious() const
{
    return m_canGoPrevious;
}

bool MprisRemotePlayer::canGoNext() const
{
    return m_canGoNext;
}

QDBusConnection &MprisRemotePlayer::dbus()
{
    return m_dbusConnection;
}

QString MprisRemotePlayer::albumArtUrl() const
{
    return m_albumArtUrl;
}

QUrl MprisRemotePlayer::localAlbumArtUrl() const
{
    return m_localAlbumArtUrl ? m_localAlbumArtUrl->localPath : QUrl{};
}

void MprisRemotePlayer::albumArtFetched(const QString &player, const QString &remoteUrl, const QSharedPointer<LocalFile> &localPath)
{
    if (identity() != player) {
        // doesn't concern us
        return;
    }
    if (albumArtUrl() != remoteUrl) {
        // doesn't concern us
        return;
    }
    Q_ASSERT(localPath);
    if (localAlbumArtUrl() == localPath->localPath) {
        // already set
        return;
    }
    setLocalAlbumArtUrl(localPath);
}

#include "moc_mprisremoteplayer.cpp"
