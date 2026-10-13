/**
 * SPDX-FileCopyrightText: 2026 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mpriscontrolplugin-macos.h"

#include "plugin_mpriscontrol_debug.h"

#include <KLocalizedString>
#include <KPluginFactory>

#include <ApplicationServices/ApplicationServices.h>
#include <IOKit/hidsystem/ev_keymap.h>
#import <AppKit/AppKit.h>

K_PLUGIN_CLASS_WITH_JSON(MprisControlPlugin, "kdeconnect_mpriscontrol.json")

namespace
{
const QString DEFAULT_PLAYER =
    i18nc("@title Users select this to control the current media player when we can't detect a specific player name like VLC", "Current Player");

void sendMediaKey(int key)
{
    for (bool down : {true, false}) {
        const int flags = down ? 0xa00 : 0xb00;
        NSEvent *event = [NSEvent otherEventWithType:NSEventTypeSystemDefined
                                            location:NSZeroPoint
                                       modifierFlags:flags
                                           timestamp:0
                                        windowNumber:0
                                             context:nil
                                             subtype:8 // NX_SUBTYPE_AUX_CONTROL_BUTTONS
                                               data1:(key << 16) | flags
                                               data2:-1];
        CGEventPost(kCGHIDEventTap, [event CGEvent]);
    }
}
}

MprisControlPlugin::MprisControlPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
{
    // Ask user to enable accessibility API
    NSDictionary *options = @{(id)kAXTrustedCheckOptionPrompt: @YES};
    if (!AXIsProcessTrustedWithOptions((CFDictionaryRef)options)) {
        qCWarning(KDECONNECT_PLUGIN_MPRISCONTROL) << "kdeconnectd is not set as a trusted accessibility, media controls will not work";
    }
}

void MprisControlPlugin::sendPlayerList()
{
    NetworkPacket np(PACKET_TYPE_MPRIS);
    np.set(QStringLiteral("playerList"), QStringList(DEFAULT_PLAYER));
    np.set(QStringLiteral("supportAlbumArtPayload"), false);
    sendPacket(np);
}

void MprisControlPlugin::receivePacket(const NetworkPacket &np)
{
    const QString name = np.get<QString>(QStringLiteral("player"));
    const bool validPlayer = (name == DEFAULT_PLAYER);
    if (!validPlayer || np.get<bool>(QStringLiteral("requestPlayerList"))) {
        sendPlayerList();
        if (!validPlayer) {
            return;
        }
    }

    if (np.has(QStringLiteral("action"))) {
        const QString &action = np.get<QString>(QStringLiteral("action"));
        if (action == QStringLiteral("PlayPause") || action == QStringLiteral("Play") || action == QStringLiteral("Pause")) {
            sendMediaKey(NX_KEYTYPE_PLAY);
        } else if (action == QStringLiteral("Next")) {
            sendMediaKey(NX_KEYTYPE_NEXT);
        } else if (action == QStringLiteral("Previous")) {
            sendMediaKey(NX_KEYTYPE_PREVIOUS);
        } else {
            qCDebug(KDECONNECT_PLUGIN_MPRISCONTROL) << "Unsupported action" << action;
        }
    }

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
        answer.set(QStringLiteral("volume"), -1);
        somethingToSend = true;
    }

    if (somethingToSend) {
        sendPacket(answer);
    }
}

#include "mpriscontrolplugin-macos.moc"
