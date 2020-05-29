/**
 * Copyright 2018 Jun Bo Bi <jambonmcyeah@gmail.com>
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

#ifndef MPRISCONTROLPLUGINWIN_H
#define MPRISCONTROLPLUGINWIN_H

#include <core/kdeconnectplugin.h>

#include <variant>

#include <QHash>
#include <vector>

#include <winrt/Windows.Media.Control.h>
#include <winrt/Windows.Storage.Streams.h>

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
