/**
 * SPDX-FileCopyrightText: 2026 LM. Garret <lm@codingarret.dev>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mprisremoteplayernowplaying-macos.h"
#include "mprisremoteplayer.h"
#include "mprisremoteplugin.h"

#include <QPointer>

#import <AppKit/AppKit.h>
#import <MediaPlayer/MediaPlayer.h>

namespace
{
QPointer<MprisRemotePlayerNowPlaying> s_active;

void addActionTarget(MPRemoteCommand *command, const QString &action)
{
    // Blocks capture C++ references by reference, so capture a copy
    const QString actionCopy = action;
    [command addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
        Q_UNUSED(event);
        if (!s_active) {
            return MPRemoteCommandHandlerStatusNoActionableNowPlayingItem;
        }
        s_active->sendAction(actionCopy);
        return MPRemoteCommandHandlerStatusSuccess;
    }];
}

void installCommandTargets()
{
    static bool installed = false;
    if (installed) {
        return;
    }
    installed = true;

    MPRemoteCommandCenter *cc = [MPRemoteCommandCenter sharedCommandCenter];
    addActionTarget(cc.playCommand, QStringLiteral("Play"));
    addActionTarget(cc.pauseCommand, QStringLiteral("Pause"));
    addActionTarget(cc.togglePlayPauseCommand, QStringLiteral("PlayPause"));
    addActionTarget(cc.nextTrackCommand, QStringLiteral("Next"));
    addActionTarget(cc.previousTrackCommand, QStringLiteral("Previous"));
    [cc.changePlaybackPositionCommand addTargetWithHandler:^MPRemoteCommandHandlerStatus(MPRemoteCommandEvent *event) {
        if (!s_active) {
            return MPRemoteCommandHandlerStatusNoActionableNowPlayingItem;
        }
        auto *positionEvent = static_cast<MPChangePlaybackPositionCommandEvent *>(event);
        s_active->setPosition(static_cast<qint64>(positionEvent.positionTime * 1000.0));
        return MPRemoteCommandHandlerStatusSuccess;
    }];
}
}

MprisRemotePlayerNowPlaying::MprisRemotePlayerNowPlaying(MprisRemotePlayer *parent, MprisRemotePlugin *plugin)
    : QObject(parent)
    , m_parent(parent)
    , m_plugin(plugin)
{
    installCommandTargets();

    connect(m_parent, &MprisRemotePlayer::playingChanged, this, &MprisRemotePlayerNowPlaying::onChanged);
    connect(m_parent, &MprisRemotePlayer::trackInfoChanged, this, &MprisRemotePlayerNowPlaying::onChanged);
    connect(m_parent, &MprisRemotePlayer::positionChanged, this, &MprisRemotePlayerNowPlaying::onChanged);
    connect(m_parent, &MprisRemotePlayer::controlsChanged, this, &MprisRemotePlayerNowPlaying::onChanged);
}

MprisRemotePlayerNowPlaying::~MprisRemotePlayerNowPlaying()
{
    if (s_active == this) {
        s_active = nullptr;
        MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
        center.nowPlayingInfo = nil;
        center.playbackState = MPNowPlayingPlaybackStateStopped;
    }
}

void MprisRemotePlayerNowPlaying::sendAction(const QString &action)
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->sendAction(action);
}

void MprisRemotePlayerNowPlaying::setPosition(qint64 positionMs)
{
    m_plugin->setPlayer(m_parent->identity());
    m_plugin->setPosition(positionMs);
    update();
}

void MprisRemotePlayerNowPlaying::onChanged()
{
    // A playing player takes over the slot, an idle one only fills an empty slot
    if (!s_active || m_parent->playing()) {
        s_active = this;
    }
    if (s_active == this) {
        update();
    }
}

void MprisRemotePlayerNowPlaying::update()
{
    MPRemoteCommandCenter *cc = [MPRemoteCommandCenter sharedCommandCenter];
    cc.playCommand.enabled = m_parent->canPlay();
    cc.pauseCommand.enabled = m_parent->canPause();
    cc.togglePlayPauseCommand.enabled = m_parent->canPlay() || m_parent->canPause();
    cc.nextTrackCommand.enabled = m_parent->canGoNext();
    cc.previousTrackCommand.enabled = m_parent->canGoPrevious();
    cc.changePlaybackPositionCommand.enabled = m_parent->canSeek();

    // Show the source app on the phone (e.g. "Deezer") next to the album
    const QString album = m_parent->album();
    const QString player = m_parent->identity();
    const QString albumLine = album.isEmpty() ? player : album + QStringLiteral(" - ") + player;

    NSMutableDictionary *info = [NSMutableDictionary dictionary];
    info[MPMediaItemPropertyTitle] = m_parent->title().toNSString();
    info[MPMediaItemPropertyArtist] = m_parent->artist().toNSString();
    info[MPMediaItemPropertyAlbumTitle] = albumLine.toNSString();
    info[MPMediaItemPropertyPlaybackDuration] = @(m_parent->length() / 1000.0);
    info[MPNowPlayingInfoPropertyElapsedPlaybackTime] = @(m_parent->position() / 1000.0);
    info[MPNowPlayingInfoPropertyPlaybackRate] = @(m_parent->playing() ? 1.0 : 0.0);
    info[MPNowPlayingInfoPropertyMediaType] = @(MPNowPlayingInfoMediaTypeAudio);

    const QUrl artUrl = m_parent->localAlbumArtUrl();
    if (artUrl.isLocalFile()) {
        NSImage *image = [[NSImage alloc] initWithContentsOfFile:artUrl.toLocalFile().toNSString()];
        if (image && image.isValid) {
            MPMediaItemArtwork *artwork = [[MPMediaItemArtwork alloc] initWithBoundsSize:image.size
                                                                          requestHandler:^NSImage *(CGSize size) {
                                                                              Q_UNUSED(size);
                                                                              return image;
                                                                          }];
            info[MPMediaItemPropertyArtwork] = artwork;
            [artwork release];
        }
        [image release];
    }

    MPNowPlayingInfoCenter *center = [MPNowPlayingInfoCenter defaultCenter];
    center.nowPlayingInfo = info;
    center.playbackState = m_parent->playing() ? MPNowPlayingPlaybackStatePlaying : MPNowPlayingPlaybackStatePaused;
}
