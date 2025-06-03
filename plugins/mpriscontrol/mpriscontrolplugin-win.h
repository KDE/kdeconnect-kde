/**
 * SPDX-FileCopyrightText: 2018 Jun Bo Bi <jambonmcyeah@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <core/kdeconnectplugin.h>

#include <variant>

#include <QBuffer>
#include <QHash>
#include <vector>

#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::Storage::Streams;
using namespace Windows::ApplicationModel;
using namespace Windows::Foundation;

#define PACKET_TYPE_MPRIS QStringLiteral("kdeconnect.mpris")

class MprisControlPlugin : public KdeConnectPlugin
{
    Q_OBJECT

public:
    explicit MprisControlPlugin(QObject *parent, const QVariantList &args);

    void receivePacket(const NetworkPacket &np) override;

public Q_SLOTS:
    bool sendAlbumArt(const std::variant<NetworkPacket, QString> &packetOrName, const QSharedPointer<QBuffer> qdata, const QString artUrl);

private:
    QHash<QString, GlobalSystemMediaTransportControlsSession> playerList;
    GlobalSystemMediaTransportControlsSessionManager sessionManager;
    std::vector<GlobalSystemMediaTransportControlsSession::PlaybackInfoChanged_revoker> playbackInfoChangedHandlers;
    std::vector<GlobalSystemMediaTransportControlsSession::MediaPropertiesChanged_revoker> mediaPropertiesChangedHandlers;
    std::vector<GlobalSystemMediaTransportControlsSession::TimelinePropertiesChanged_revoker> timelinePropertiesChangedHandlers;

    std::optional<QString> getPlayerName(const GlobalSystemMediaTransportControlsSession &player);
    TimeSpan getPlayerPosition(const GlobalSystemMediaTransportControlsSession &player);
    void sendMediaProperties(const std::variant<NetworkPacket, QString> &packetOrName, const GlobalSystemMediaTransportControlsSession &player);
    void sendPlaybackInfo(const std::variant<NetworkPacket, QString> &packetOrName, const GlobalSystemMediaTransportControlsSession &player);
    void sendTimelineProperties(const std::variant<NetworkPacket, QString> &packetOrName,
                                const GlobalSystemMediaTransportControlsSession &player,
                                bool lengthOnly = false);
    void updatePlayerList();
    void sendPlayerList();
    void getThumbnail(const std::variant<NetworkPacket, QString> &packetOrName, const GlobalSystemMediaTransportControlsSession &player, const QString artUrl);

    void handleDefaultPlayer(const NetworkPacket &np);

    QString randomUrl();
};
