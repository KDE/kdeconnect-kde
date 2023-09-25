/**
 * SPDX-FileCopyrightText: 2018 Jun Bo Bi <jambonmcyeah@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mpriscontrolplugin-win.h"

#include "plugin_mpriscontrol_debug.h"
#include <core/device.h>

#include <KLocalizedString>
#include <KPluginFactory>

#include <QBuffer>

#include <chrono>
#include <random>

#include <ppltasks.h>
#include <windows.h>

using namespace Windows::Foundation;

K_PLUGIN_CLASS_WITH_JSON(MprisControlPlugin, "kdeconnect_mpriscontrol.json")

namespace
{
const QString DEFAULT_PLAYER =
    i18nc("@title Users select this to control the current media player when we can't detect a specific player name like VLC", "Current Player");
}

MprisControlPlugin::MprisControlPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , sessionManager(GlobalSystemMediaTransportControlsSessionManager::RequestAsync().get())
{
    sessionManager.SessionsChanged([this](GlobalSystemMediaTransportControlsSessionManager, SessionsChangedEventArgs) {
        this->updatePlayerList();
    });
    this->updatePlayerList();
}

std::optional<QString> MprisControlPlugin::getPlayerName(GlobalSystemMediaTransportControlsSession const &player)
{
    auto entry = std::find(this->playerList.constBegin(), this->playerList.constEnd(), player);

    if (entry == this->playerList.constEnd()) {
        qCWarning(KDECONNECT_PLUGIN_MPRISCONTROL) << "PlaybackInfoChanged received for no longer tracked session" << player.SourceAppUserModelId().c_str();
        return std::nullopt;
    }

    return entry.key();
}

QString MprisControlPlugin::randomUrl()
{
    const QString VALID_CHARS = QStringLiteral("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ");
    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, VALID_CHARS.size() - 1);

    const int size = 10;
    QString fileUrl(size, QChar());
    for (int i = 0; i < size; i++) {
        fileUrl[i] = VALID_CHARS[distribution(generator)];
    }

    return QStringLiteral("file://") + fileUrl;
}

void MprisControlPlugin::sendMediaProperties(std::variant<NetworkPacket, QString> const &packetOrName, GlobalSystemMediaTransportControlsSession const &player)
{
    NetworkPacket np = packetOrName.index() == 0 ? std::get<0>(packetOrName) : NetworkPacket(PACKET_TYPE_MPRIS);
    if (packetOrName.index() == 1)
        np.set(QStringLiteral("player"), std::get<1>(packetOrName));

    auto mediaProperties = player.TryGetMediaPropertiesAsync().get();

    np.set(QStringLiteral("title"), QString::fromWCharArray(mediaProperties.Title().c_str()));
    np.set(QStringLiteral("artist"), QString::fromWCharArray(mediaProperties.Artist().c_str()));
    np.set(QStringLiteral("album"), QString::fromWCharArray(mediaProperties.AlbumTitle().c_str()));
    np.set(QStringLiteral("albumArtUrl"), randomUrl());

    np.set(QStringLiteral("url"), QString());
    sendTimelineProperties(np, player, true); // "length"

    if (packetOrName.index() == 1)
        sendPacket(np);
}

void MprisControlPlugin::sendPlaybackInfo(std::variant<NetworkPacket, QString> const &packetOrName, GlobalSystemMediaTransportControlsSession const &player)
{
    NetworkPacket np = packetOrName.index() == 0 ? std::get<0>(packetOrName) : NetworkPacket(PACKET_TYPE_MPRIS);
    if (packetOrName.index() == 1)
        np.set(QStringLiteral("player"), std::get<1>(packetOrName));

    sendMediaProperties(np, player);

    auto playbackInfo = player.GetPlaybackInfo();
    auto playbackControls = playbackInfo.Controls();

    np.set(QStringLiteral("isPlaying"), playbackInfo.PlaybackStatus() == GlobalSystemMediaTransportControlsSessionPlaybackStatus::Playing);
    np.set(QStringLiteral("canPause"), playbackControls.IsPauseEnabled());
    np.set(QStringLiteral("canPlay"), playbackControls.IsPlayEnabled());
    np.set(QStringLiteral("canGoNext"), playbackControls.IsNextEnabled());
    np.set(QStringLiteral("canGoPrevious"), playbackControls.IsPreviousEnabled());
    np.set(QStringLiteral("canSeek"), playbackControls.IsPlaybackPositionEnabled());

    if (playbackInfo.IsShuffleActive()) {
        const bool shuffleStatus = playbackInfo.IsShuffleActive().Value();
        np.set(QStringLiteral("shuffle"), shuffleStatus);
    }

    if (playbackInfo.AutoRepeatMode()) {
        QString loopStatus;
        switch (playbackInfo.AutoRepeatMode().Value()) {
        case Windows::Media::MediaPlaybackAutoRepeatMode::List: {
            loopStatus = QStringLiteral("Playlist");
            break;
        }
        case Windows::Media::MediaPlaybackAutoRepeatMode::Track: {
            loopStatus = QStringLiteral("Track");
            break;
        }
        default: {
            loopStatus = QStringLiteral("None");
            break;
        }
        }
        np.set(QStringLiteral("loopStatus"), loopStatus);
    }

    sendTimelineProperties(np, player);

    if (packetOrName.index() == 1)
        sendPacket(np);
}

void MprisControlPlugin::sendTimelineProperties(std::variant<NetworkPacket, QString> const &packetOrName,
                                                GlobalSystemMediaTransportControlsSession const &player,
                                                bool lengthOnly)
{
    NetworkPacket np = packetOrName.index() == 0 ? std::get<0>(packetOrName) : NetworkPacket(PACKET_TYPE_MPRIS);
    if (packetOrName.index() == 1)
        np.set(QStringLiteral("player"), std::get<1>(packetOrName));

    auto timelineProperties = player.GetTimelineProperties();

    if (!lengthOnly) {
        const auto playbackInfo = player.GetPlaybackInfo();
        const auto playbackControls = playbackInfo.Controls();
        np.set(QStringLiteral("canSeek"), playbackControls.IsPlaybackPositionEnabled());
        np.set(QStringLiteral("pos"),
               std::chrono::duration_cast<std::chrono::milliseconds>(timelineProperties.Position() - timelineProperties.StartTime()).count());
    }
    np.set(QStringLiteral("length"),
           std::chrono::duration_cast<std::chrono::milliseconds>(timelineProperties.EndTime() - timelineProperties.StartTime()).count());

    if (packetOrName.index() == 1)
        sendPacket(np);
}

void MprisControlPlugin::updatePlayerList()
{
    playerList.clear();
    playbackInfoChangedHandlers.clear();
    mediaPropertiesChangedHandlers.clear();
    timelinePropertiesChangedHandlers.clear();

    auto sessions = sessionManager.GetSessions();
    playbackInfoChangedHandlers.resize(sessions.Size());
    mediaPropertiesChangedHandlers.resize(sessions.Size());
    timelinePropertiesChangedHandlers.resize(sessions.Size());

    for (uint32_t i = 0; i < sessions.Size(); i++) {
        const auto player = sessions.GetAt(i);
        auto playerName = player.SourceAppUserModelId();

#if WIN_SDK_VERSION >= 19041
        // try to resolve the AUMID to a user-friendly name
        try {
            playerName = AppInfo::GetFromAppUserModelId(playerName).DisplayInfo().DisplayName();
        } catch (winrt::hresult_error e) {
            qCDebug(KDECONNECT_PLUGIN_MPRISCONTROL) << QString::fromWCharArray(playerName.c_str()) << "doesn\'t have a valid AppUserModelID! Sending as-is..";
        }
#endif
        QString uniqueName = QString::fromWCharArray(playerName.c_str());
        for (int i = 2; playerList.contains(uniqueName); ++i) {
            uniqueName += QStringLiteral(" [") + QString::number(i) + QStringLiteral("]");
        }

        playerList.insert(uniqueName, player);

        player
            .PlaybackInfoChanged(auto_revoke,
                                 [this](GlobalSystemMediaTransportControlsSession player, PlaybackInfoChangedEventArgs args) {
                                     if (auto name = getPlayerName(player))
                                         this->sendPlaybackInfo(name.value(), player);
                                 })
            .swap(playbackInfoChangedHandlers[i]);
        concurrency::create_task([this, player] {
            std::chrono::milliseconds timespan(50);
            std::this_thread::sleep_for(timespan);

            if (auto name = getPlayerName(player))
                this->sendPlaybackInfo(name.value(), player);
        });

        if (auto name = getPlayerName(player))
            sendPlaybackInfo(name.value(), player);

        player
            .MediaPropertiesChanged(auto_revoke,
                                    [this](GlobalSystemMediaTransportControlsSession player, MediaPropertiesChangedEventArgs args) {
                                        if (auto name = getPlayerName(player))
                                            this->sendMediaProperties(name.value(), player);
                                    })
            .swap(mediaPropertiesChangedHandlers[i]);
        concurrency::create_task([this, player] {
            std::chrono::milliseconds timespan(50);
            std::this_thread::sleep_for(timespan);

            if (auto name = getPlayerName(player))
                this->sendMediaProperties(name.value(), player);
        });

        player
            .TimelinePropertiesChanged(auto_revoke,
                                       [this](GlobalSystemMediaTransportControlsSession player, TimelinePropertiesChangedEventArgs args) {
                                           if (auto name = getPlayerName(player))
                                               this->sendTimelineProperties(name.value(), player);
                                       })
            .swap(timelinePropertiesChangedHandlers[i]);
        concurrency::create_task([this, player] {
            std::chrono::milliseconds timespan(50);
            std::this_thread::sleep_for(timespan);

            if (auto name = getPlayerName(player))
                this->sendTimelineProperties(name.value(), player);
        });
    }

    sendPlayerList();
}

void MprisControlPlugin::sendPlayerList()
{
    NetworkPacket np(PACKET_TYPE_MPRIS);

    np.set(QStringLiteral("playerList"), playerList.keys() + QStringList(DEFAULT_PLAYER));
    np.set(QStringLiteral("supportAlbumArtPayload"), false); // TODO: Sending albumArt doesn't work

    sendPacket(np);
}

bool MprisControlPlugin::sendAlbumArt(std::variant<NetworkPacket, QString> const &packetOrName,
                                      GlobalSystemMediaTransportControlsSession const &player,
                                      QString artUrl)
{
    qWarning(KDECONNECT_PLUGIN_MPRISCONTROL) << "Sending Album Art";
    NetworkPacket np = packetOrName.index() == 0 ? std::get<0>(packetOrName) : NetworkPacket(PACKET_TYPE_MPRIS);
    if (packetOrName.index() == 1)
        np.set(QStringLiteral("player"), std::get<1>(packetOrName));

    auto thumbnail = player.TryGetMediaPropertiesAsync().get().Thumbnail();
    if (thumbnail) {
        auto stream = thumbnail.OpenReadAsync().get();
        if (stream && stream.CanRead()) {
            IBuffer data = Buffer(stream.Size());
            data = stream.ReadAsync(data, stream.Size(), InputStreamOptions::None).get();
            QSharedPointer<QBuffer> qdata = QSharedPointer<QBuffer>(new QBuffer());
            qdata->setData((char *)data.data(), data.Capacity());

            np.set(QStringLiteral("transferringAlbumArt"), true);
            np.set(QStringLiteral("albumArtUrl"), artUrl);

            np.setPayload(qdata, qdata->size());

            if (packetOrName.index() == 1)
                sendPacket(np);

            return true;
        }

        return false;
    } else {
        return false;
    }
}

void MprisControlPlugin::handleDefaultPlayer(const NetworkPacket &np)
{
    if (np.has(QStringLiteral("action"))) {
        INPUT input = {0};
        input.type = INPUT_KEYBOARD;

        input.ki.time = 0;
        input.ki.dwExtraInfo = 0;
        input.ki.wScan = 0;
        input.ki.dwFlags = 0;

        const QString &action = np.get<QString>(QStringLiteral("action"));

        if (action == QStringLiteral("PlayPause") || (action == QStringLiteral("Play")) || (action == QStringLiteral("Pause"))) {
            input.ki.wVk = VK_MEDIA_PLAY_PAUSE;
            ::SendInput(1, &input, sizeof(INPUT));
        } else if (action == QStringLiteral("Stop")) {
            input.ki.wVk = VK_MEDIA_STOP;
            ::SendInput(1, &input, sizeof(INPUT));
        } else if (action == QStringLiteral("Next")) {
            input.ki.wVk = VK_MEDIA_NEXT_TRACK;
            ::SendInput(1, &input, sizeof(INPUT));
        } else if (action == QStringLiteral("Previous")) {
            input.ki.wVk = VK_MEDIA_PREV_TRACK;
            ::SendInput(1, &input, sizeof(INPUT));
        } else if (action == QStringLiteral("Stop")) {
            input.ki.wVk = VK_MEDIA_STOP;
            ::SendInput(1, &input, sizeof(INPUT));
        }
    }

    // Send something read from the mpris interface
    NetworkPacket answer(PACKET_TYPE_MPRIS);
    answer.set(QStringLiteral("player"), DEFAULT_PLAYER);
    bool somethingToSend = false;
    if (np.get<bool>(QStringLiteral("requestNowPlaying"))) {
        answer.set(QStringLiteral("pos"), 0);
        answer.set(QStringLiteral("isPlaying"), false);
        answer.set(QStringLiteral("canPause"), false);
        answer.set(QStringLiteral("canPlay"), true);
        answer.set(QStringLiteral("canGoNext"), true);
        answer.set(QStringLiteral("canGoPrevious"), true);
        answer.set(QStringLiteral("canSeek"), false);
        somethingToSend = true;
    }

    if (np.get<bool>(QStringLiteral("requestVolume"))) {
        // we don't support setting per-app volume levels yet
        answer.set(QStringLiteral("volume"), -1);
        somethingToSend = true;
    }

    if (somethingToSend) {
        sendPacket(answer);
    }
}

void MprisControlPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.has(QStringLiteral("playerList"))) {
        return; // Whoever sent this is an mpris client and not an mpris control!
    }

    // Send the player list
    const QString name = np.get<QString>(QStringLiteral("player"));

    if (name == DEFAULT_PLAYER) {
        handleDefaultPlayer(np);
        return;
    }

    auto it = playerList.find(name);
    bool valid_player = (it != playerList.end());
    if (!valid_player || np.get<bool>(QStringLiteral("requestPlayerList"))) {
        updatePlayerList();
        sendPlayerList();
        if (!valid_player) {
            return;
        }
    }

    auto player = it.value();

    if (np.has(QStringLiteral("albumArtUrl"))) {
        sendAlbumArt(name, player, np.get<QString>(QStringLiteral("albumArtUrl")));
        return;
    }

    if (np.has(QStringLiteral("action"))) {
        const QString &action = np.get<QString>(QStringLiteral("action"));
        if (action == QStringLiteral("Next")) {
            player.TrySkipNextAsync().get();
        } else if (action == QStringLiteral("Previous")) {
            player.TrySkipPreviousAsync().get();
        } else if (action == QStringLiteral("Pause")) {
            player.TryPauseAsync().get();
        } else if (action == QStringLiteral("PlayPause")) {
            player.TryTogglePlayPauseAsync().get();
        } else if (action == QStringLiteral("Stop")) {
            player.TryStopAsync().get();
        } else if (action == QStringLiteral("Play")) {
            player.TryPlayAsync().get();
        }
    }
    if (np.has(QStringLiteral("setVolume"))) {
        qWarning(KDECONNECT_PLUGIN_MPRISCONTROL) << "Setting volume is not supported";
    }
    if (np.has(QStringLiteral("Seek"))) {
        TimeSpan offset = std::chrono::microseconds(np.get<int>(QStringLiteral("Seek")));
        qWarning(KDECONNECT_PLUGIN_MPRISCONTROL) << "Seeking" << offset.count() << "ns to" << name;
        player.TryChangePlaybackPositionAsync((player.GetTimelineProperties().Position() + offset).count()).get();
    }

    if (np.has(QStringLiteral("SetPosition"))) {
        TimeSpan position = std::chrono::milliseconds(np.get<qlonglong>(QStringLiteral("SetPosition"), 0));
        player.TryChangePlaybackPositionAsync((player.GetTimelineProperties().StartTime() + position).count()).get();
    }

    if (np.has(QStringLiteral("setShuffle"))) {
        player.TryChangeShuffleActiveAsync(np.get<bool>(QStringLiteral("setShuffle")));
    }

    if (np.has(QStringLiteral("setLoopStatus"))) {
        QString loopStatus = np.get<QString>(QStringLiteral("setLoopStatus"));
        enum class winrt::Windows::Media::MediaPlaybackAutoRepeatMode loopStatusEnumVal;
        if (loopStatus == QStringLiteral("Track")) {
            loopStatusEnumVal = Windows::Media::MediaPlaybackAutoRepeatMode::Track;
        } else if (loopStatus == QStringLiteral("Playlist")) {
            loopStatusEnumVal = Windows::Media::MediaPlaybackAutoRepeatMode::List;
        } else {
            loopStatusEnumVal = Windows::Media::MediaPlaybackAutoRepeatMode::None;
        }
        player.TryChangeAutoRepeatModeAsync(loopStatusEnumVal);
    }

    // Send something read from the mpris interface
    NetworkPacket answer(PACKET_TYPE_MPRIS);
    answer.set(QStringLiteral("player"), name);
    bool somethingToSend = false;
    if (np.get<bool>(QStringLiteral("requestNowPlaying"))) {
        sendPlaybackInfo(answer, player);
        somethingToSend = true;
    }
    if (np.get<bool>(QStringLiteral("requestVolume"))) {
        // we don't support setting per-app volume levels yet
        answer.set(QStringLiteral("volume"), -1);
        somethingToSend = true;
    }

    if (somethingToSend) {
        sendPacket(answer);
    }
}

#include "moc_mpriscontrolplugin-win.cpp"
#include "mpriscontrolplugin-win.moc"
