/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pausemusicplugin.h"

#include <KPluginFactory>
#include <PulseAudioQt/Context>
#include <PulseAudioQt/Sink>

#include "generated/systeminterfaces/mprisplayer.h"
#include <dbushelper.h>

#include "plugin_pausemusic_debug.h"

K_PLUGIN_CLASS_WITH_JSON(PauseMusicPlugin, "kdeconnect_pausemusic.json")

PauseMusicPlugin::PauseMusicPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , mutedSinks()
{
}

void PauseMusicPlugin::receivePacket(const NetworkPacket &np)
{
    bool pauseOnlyWhenTalking = config()->getBool(QStringLiteral("conditionTalking"), false);

    if (pauseOnlyWhenTalking) {
        if (np.get<QString>(QStringLiteral("event")) != QLatin1String("talking")) {
            return;
        }
    } else { // Pause as soon as it rings
        if (np.get<QString>(QStringLiteral("event")) != QLatin1String("ringing") && np.get<QString>(QStringLiteral("event")) != QLatin1String("talking")) {
            return;
        }
    }

    bool pauseConditionFulfilled = !np.get<bool>(QStringLiteral("isCancel"));

    bool pause = config()->getBool(QStringLiteral("actionPause"), true);
    bool mute = config()->getBool(QStringLiteral("actionMute"), false);

    const bool autoResume = config()->getBool(QStringLiteral("actionResume"), true);

    if (pauseConditionFulfilled) {
        if (mute) {
            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Muting system volume";
            const auto sinks = PulseAudioQt::Context::instance()->sinks();
            for (const auto sink : sinks) {
                if (!sink->isMuted()) {
                    sink->setMuted(true);
                    mutedSinks.insert(sink->name());
                }
            }
        }

        if (pause) {
            // Search for interfaces currently playing
            const QStringList interfaces = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
            for (const QString &iface : interfaces) {
                if (iface.startsWith(QLatin1String("org.mpris.MediaPlayer2"))) {
                    OrgMprisMediaPlayer2PlayerInterface mprisInterface(iface, QStringLiteral("/org/mpris/MediaPlayer2"), QDBusConnection::sessionBus());
                    QString status = mprisInterface.playbackStatus();
                    if (status == QLatin1String("Playing")) {
                        if (!pausedSources.contains(iface)) {
                            pausedSources.insert(iface);
                            if (mprisInterface.canPause()) {
                                mprisInterface.Pause();
                            } else {
                                mprisInterface.Stop();
                            }
                        }
                    }
                }
            }
        }

    } else {
        if (mute) {
            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Unmuting system volume";

            if (autoResume) {
                const auto sinks = PulseAudioQt::Context::instance()->sinks();
                for (const auto sink : sinks) {
                    if (mutedSinks.contains(sink->name())) {
                        sink->setMuted(false);
                    }
                }
            }
            mutedSinks.clear();
        }

        if (pause && !pausedSources.empty()) {
            if (autoResume) {
                for (const QString &iface : std::as_const(pausedSources)) {
                    OrgMprisMediaPlayer2PlayerInterface mprisInterface(iface, QStringLiteral("/org/mpris/MediaPlayer2"), QDBusConnection::sessionBus());
                    mprisInterface.Play();
                }
            }
            pausedSources.clear();
        }
    }
}

#include "moc_pausemusicplugin.cpp"
#include "pausemusicplugin.moc"
