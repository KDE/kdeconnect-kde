/**
 * SPDX-FileCopyrightText: 2019 Matthijs Tijink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mprisremoteplayermediaplayer2player.h"
#include "mprisremoteplayer.h"
#include "mprisremoteplugin.h"

#include <QDBusMessage>

MprisRemotePlayerMediaPlayer2Player::MprisRemotePlayerMediaPlayer2Player(MprisRemotePlayer *parent, MprisRemotePlugin *plugin)
    : QDBusAbstractAdaptor{parent}
    , m_parent{parent}
    , m_plugin{plugin}
    , m_controlsChanged{false}
    , m_trackInfoChanged{false}
    , m_positionChanged{false}
    , m_volumeChanged{false}
    , m_playingChanged{false}
{
    connect(m_parent, &MprisRemotePlayer::controlsChanged, this, &MprisRemotePlayerMediaPlayer2Player::controlsChanged);
    connect(m_parent, &MprisRemotePlayer::trackInfoChanged, this, &MprisRemotePlayerMediaPlayer2Player::trackInfoChanged);
    connect(m_parent, &MprisRemotePlayer::positionChanged, this, &MprisRemotePlayerMediaPlayer2Player::positionChanged);
    connect(m_parent, &MprisRemotePlayer::volumeChanged, this, &MprisRemotePlayerMediaPlayer2Player::volumeChanged);
    connect(m_parent, &MprisRemotePlayer::playingChanged, this, &MprisRemotePlayerMediaPlayer2Player::playingChanged);
}

QString MprisRemotePlayerMediaPlayer2Player::PlaybackStatus() const
{
    if (m_parent->playing()) {
        return QStringLiteral("Playing");
    } else {
        return QStringLiteral("Paused");
    }
}

double MprisRemotePlayerMediaPlayer2Player::Rate() const
{
    return 1.0;
}

QVariantMap MprisRemotePlayerMediaPlayer2Player::Metadata() const
{
    QVariantMap metadata;
    metadata[QStringLiteral("mpris:trackid")] = QVariant::fromValue<QDBusObjectPath>(QDBusObjectPath("/org/mpris/MediaPlayer2"));

    if (m_parent->length() > 0) {
        metadata[QStringLiteral("mpris:length")] = QVariant::fromValue<qlonglong>(m_parent->length() * qlonglong(1000));
    }
    if (!m_parent->title().isEmpty()) {
        metadata[QStringLiteral("xesam:title")] = m_parent->title();
    }
    if (!m_parent->artist().isEmpty()) {
        metadata[QStringLiteral("xesam:artist")] = QStringList{m_parent->artist()};
    }
    if (!m_parent->album().isEmpty()) {
        metadata[QStringLiteral("xesam:album")] = m_parent->album();
    }
    if (!m_parent->localAlbumArtUrl().isEmpty()) {
        metadata[QStringLiteral("mpris:artUrl")] = m_parent->localAlbumArtUrl().toString();
    }
    return metadata;
}

double MprisRemotePlayerMediaPlayer2Player::Volume() const
{
    return m_parent->volume() / 100.0;
}

void MprisRemotePlayerMediaPlayer2Player::setVolume(double volume) const
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->setVolume(volume * 100.0 + 0.5);
}

qlonglong MprisRemotePlayerMediaPlayer2Player::Position() const
{
    return m_plugin->position() * qlonglong(1000);
}

double MprisRemotePlayerMediaPlayer2Player::MinimumRate() const
{
    return 1.0;
}

double MprisRemotePlayerMediaPlayer2Player::MaximumRate() const
{
    return 1.0;
}

bool MprisRemotePlayerMediaPlayer2Player::CanGoNext() const
{
    return m_parent->canGoNext();
}

bool MprisRemotePlayerMediaPlayer2Player::CanGoPrevious() const
{
    return m_parent->canGoPrevious();
}

bool MprisRemotePlayerMediaPlayer2Player::CanPlay() const
{
    return m_parent->canPlay();
}

bool MprisRemotePlayerMediaPlayer2Player::CanPause() const
{
    return m_parent->canPause();
}

bool MprisRemotePlayerMediaPlayer2Player::CanSeek() const
{
    return m_parent->canSeek();
}

bool MprisRemotePlayerMediaPlayer2Player::CanControl() const
{
    return true;
}

void MprisRemotePlayerMediaPlayer2Player::Next()
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->sendAction(QStringLiteral("Next"));
}

void MprisRemotePlayerMediaPlayer2Player::Previous()
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->sendAction(QStringLiteral("Previous"));
}

void MprisRemotePlayerMediaPlayer2Player::Pause()
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->sendAction(QStringLiteral("Pause"));
}

void MprisRemotePlayerMediaPlayer2Player::PlayPause()
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->sendAction(QStringLiteral("PlayPause"));
}

void MprisRemotePlayerMediaPlayer2Player::Stop()
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->sendAction(QStringLiteral("Stop"));
}

void MprisRemotePlayerMediaPlayer2Player::Play()
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->sendAction(QStringLiteral("Play"));
}

void MprisRemotePlayerMediaPlayer2Player::Seek(qlonglong Offset)
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->seek(Offset);
}

void MprisRemotePlayerMediaPlayer2Player::SetPosition(QDBusObjectPath /*TrackId*/, qlonglong Position)
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->setPosition(Position / 1000);
}

void MprisRemotePlayerMediaPlayer2Player::OpenUri(QString /*Uri*/)
{
}

void MprisRemotePlayerMediaPlayer2Player::controlsChanged()
{
    m_controlsChanged = true;
    QMetaObject::invokeMethod(this, &MprisRemotePlayerMediaPlayer2Player::emitPropertiesChanged, Qt::QueuedConnection);
}

void MprisRemotePlayerMediaPlayer2Player::playingChanged()
{
    m_playingChanged = true;
    QMetaObject::invokeMethod(this, &MprisRemotePlayerMediaPlayer2Player::emitPropertiesChanged, Qt::QueuedConnection);
}

void MprisRemotePlayerMediaPlayer2Player::positionChanged()
{
    m_positionChanged = true;
    QMetaObject::invokeMethod(this, &MprisRemotePlayerMediaPlayer2Player::emitPropertiesChanged, Qt::QueuedConnection);
}

void MprisRemotePlayerMediaPlayer2Player::trackInfoChanged()
{
    m_trackInfoChanged = true;
    QMetaObject::invokeMethod(this, &MprisRemotePlayerMediaPlayer2Player::emitPropertiesChanged, Qt::QueuedConnection);
}

void MprisRemotePlayerMediaPlayer2Player::volumeChanged()
{
    m_volumeChanged = true;
    QMetaObject::invokeMethod(this, &MprisRemotePlayerMediaPlayer2Player::emitPropertiesChanged, Qt::QueuedConnection);
}

void MprisRemotePlayerMediaPlayer2Player::emitPropertiesChanged()
{
    // Always invoked "queued", so we can send all changes at once
    // Check if things really changed (we might get called multiple times)
    if (!m_controlsChanged && !m_trackInfoChanged && !m_positionChanged && !m_volumeChanged && !m_playingChanged)
        return;

    // Qt doesn't automatically send the "org.freedesktop.DBus.Properties PropertiesChanged" signal, so do it manually
    // With the current setup, it's hard to discover what properties changed. So just send all properties (not too large, usually)
    QVariantMap properties;
    if (m_trackInfoChanged) {
        properties[QStringLiteral("Metadata")] = Metadata();
    }
    if (m_trackInfoChanged || m_positionChanged) {
        properties[QStringLiteral("Position")] = Position();
    }

    if (m_controlsChanged) {
        properties[QStringLiteral("CanGoNext")] = CanGoNext();
        properties[QStringLiteral("CanGoPrevious")] = CanGoPrevious();
        properties[QStringLiteral("CanPlay")] = CanPlay();
        properties[QStringLiteral("CanPause")] = CanPause();
        properties[QStringLiteral("CanSeek")] = CanSeek();
    }

    if (m_playingChanged) {
        properties[QStringLiteral("PlaybackStatus")] = PlaybackStatus();
    }

    if (m_volumeChanged) {
        properties[QStringLiteral("Volume")] = Volume();
    }

    QList<QVariant> args;
    args.push_back(QVariant(QStringLiteral("org.mpris.MediaPlayer2.Player")));
    args.push_back(properties);
    args.push_back(QStringList{});
    QDBusMessage message = QDBusMessage::createSignal(QStringLiteral("/org/mpris/MediaPlayer2"),
                                                      QStringLiteral("org.freedesktop.DBus.Properties"),
                                                      QStringLiteral("PropertiesChanged"));
    message.setArguments(args);
    // Send it over the correct DBus connection
    m_parent->dbus().send(message);

    if (m_positionChanged || m_trackInfoChanged) {
        Q_EMIT Seeked(Position());
    }

    m_controlsChanged = false;
    m_trackInfoChanged = false;
    m_playingChanged = false;
    m_positionChanged = false;
    m_volumeChanged = false;
}

#include "moc_mprisremoteplayermediaplayer2player.cpp"
