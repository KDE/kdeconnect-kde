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

#include "mpriscontrolplugin-win.h"
#include <core/device.h>
#include "plugin_mpris_debug.h"

#include <KPluginFactory>

#include <Windows.h>

K_PLUGIN_CLASS_WITH_JSON(MprisControlPlugin, "kdeconnect_mpriscontrol.json")

MprisControlPlugin::MprisControlPlugin(QObject *parent, const QVariantList &args) : KdeConnectPlugin(parent, args) {  }

bool MprisControlPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.has(QStringLiteral("playerList"))) {
        return false; //Whoever sent this is an mpris client and not an mpris control!
    }


    //Send the player list
    const QString player = np.get<QString>(QStringLiteral("player"));
    bool valid_player = (player == playername);
    if (!valid_player || np.get<bool>(QStringLiteral("requestPlayerList"))) {
        const QList<QString> playerlist = {playername};

        NetworkPacket np(PACKET_TYPE_MPRIS);
        np.set(QStringLiteral("playerList"), playerlist);
        np.set(QStringLiteral("supportAlbumArtPayload"), false);
        sendPacket(np);
        if (!valid_player) {
            return true;
        }
    }

    if (np.has(QStringLiteral("action"))) {
        INPUT input={0};
        input.type = INPUT_KEYBOARD;

        input.ki.time = 0;
        input.ki.dwExtraInfo = 0;
        input.ki.wScan = 0;
        input.ki.dwFlags = 0;

        if (np.has(QStringLiteral("action"))) {
            const QString& action = np.get<QString>(QStringLiteral("action"));
            if (action == QStringLiteral("PlayPause") || (action == QStringLiteral("Play")) || (action == QStringLiteral("Pause")) ) {
                input.ki.wVk = VK_MEDIA_PLAY_PAUSE;
                ::SendInput(1,&input,sizeof(INPUT));
            }
            else if (action == QStringLiteral("Stop")) {
                input.ki.wVk = VK_MEDIA_STOP;
                ::SendInput(1,&input,sizeof(INPUT));
            }
            else if (action == QStringLiteral("Next")) {
                    input.ki.wVk = VK_MEDIA_NEXT_TRACK;
                ::SendInput(1,&input,sizeof(INPUT));
            }
            else if (action == QStringLiteral("Previous")) {
                input.ki.wVk = VK_MEDIA_PREV_TRACK;
                ::SendInput(1,&input,sizeof(INPUT));
            }
            else if (action == QStringLiteral("Stop")) {
                input.ki.wVk = VK_MEDIA_STOP;
                ::SendInput(1,&input,sizeof(INPUT));
            }
        }

    }

    NetworkPacket answer(PACKET_TYPE_MPRIS);
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
        answer.set(QStringLiteral("volume"), 100);
        somethingToSend = true;
    }

    if (somethingToSend) {
        answer.set(QStringLiteral("player"), player);
        sendPacket(answer);
    }

    return true;
}

#include "mpriscontrolplugin-win.moc"
