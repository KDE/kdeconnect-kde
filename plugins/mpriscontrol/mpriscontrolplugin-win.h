/**
 * SPDX-FileCopyrightText: 2018 Jun Bo Bi <jambonmcyeah@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef MPRISCONTROLPLUGINWIN_H
#define MPRISCONTROLPLUGINWIN_H

#include <core/kdeconnectplugin.h>

#include <variant>

#include <QHash>
#include <vector>

#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>
#include <winrt/Windows.Foundation.Collections.h>

using namespace winrt;
using namespace Windows::Media::Control;
using namespace Windows::Storage::Streams;

#define PACKET_TYPE_MPRIS QStringLiteral("kdeconnect.mpris")

class Q_DECL_EXPORT MprisControlPlugin
    : public KdeConnectPlugin
{
    Q_OBJECT

  public:
    explicit MprisControlPlugin(QObject *parent, const QVariantList &args);

    bool receivePacket(const NetworkPacket &np) override;
    void connected() override {}

  private:
    std::optional<GlobalSystemMediaTransportControlsSessionManager> sessionManager;
    QHash<QString, GlobalSystemMediaTransportControlsSession> playerList;

    std::vector<GlobalSystemMediaTransportControlsSession::PlaybackInfoChanged_revoker> playbackInfoChangedHandlers;
    std::vector<GlobalSystemMediaTransportControlsSession::MediaPropertiesChanged_revoker> mediaPropertiesChangedHandlers;
    std::vector<GlobalSystemMediaTransportControlsSession::TimelinePropertiesChanged_revoker> timelinePropertiesChangedHandlers;

    std::optional<QString> getPlayerName(GlobalSystemMediaTransportControlsSession const& player);
    void sendMediaProperties(std::variant<NetworkPacket, QString> const& packetOrName, GlobalSystemMediaTransportControlsSession const& player);
    void sendPlaybackInfo(std::variant<NetworkPacket, QString> const& packetOrName, GlobalSystemMediaTransportControlsSession const& player);
    void sendTimelineProperties(std::variant<NetworkPacket, QString> const& packetOrName, GlobalSystemMediaTransportControlsSession const& player, bool lengthOnly = false);
    void updatePlayerList();
    void sendPlayerList();
    bool sendAlbumArt(std::variant<NetworkPacket, QString> const& packetOrName, GlobalSystemMediaTransportControlsSession const& player, QString artUrl);

    QString randomUrl();
};
#endif //MPRISCONTROLPLUGINWIN_H
