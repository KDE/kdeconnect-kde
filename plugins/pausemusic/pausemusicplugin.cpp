/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "pausemusicplugin.h"

#include <KPluginFactory>
#include <PulseAudioQt/Context>
#include <PulseAudioQt/Sink>

#include <dbushelper.h>
#include "mprisplayer.h"

//In older Qt released, qAsConst isnt available
#include "qtcompat_p.h"

K_PLUGIN_CLASS_WITH_JSON(PauseMusicPlugin, "kdeconnect_pausemusic.json")

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_PAUSEMUSIC, "kdeconnect.plugin.pausemusic")

PauseMusicPlugin::PauseMusicPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , mutedSinks()
{}

bool PauseMusicPlugin::receivePacket(const NetworkPacket& np)
{
    bool pauseOnlyWhenTalking = config()->get(QStringLiteral("conditionTalking"), false);

    if (pauseOnlyWhenTalking) {
        if (np.get<QString>(QStringLiteral("event")) != QLatin1String("talking")) {
            return true;
        }
    } else { //Pause as soon as it rings
        if (np.get<QString>(QStringLiteral("event")) != QLatin1String("ringing") && np.get<QString>(QStringLiteral("event")) != QLatin1String("talking")) {
            return true;
        }
    }

    bool pauseConditionFulfilled = !np.get<bool>(QStringLiteral("isCancel"));

    bool pause = config()->get(QStringLiteral("actionPause"), true);
    bool mute = config()->get(QStringLiteral("actionMute"), false);

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
            //Search for interfaces currently playing
            const QStringList interfaces = DBusHelper::sessionBus().interface()->registeredServiceNames().value();
            for (const QString& iface : interfaces) {
                if (iface.startsWith(QLatin1String("org.mpris.MediaPlayer2"))) {
                    OrgMprisMediaPlayer2PlayerInterface mprisInterface(iface, QStringLiteral("/org/mpris/MediaPlayer2"), DBusHelper::sessionBus());
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

            const auto sinks = PulseAudioQt::Context::instance()->sinks();
            for (const auto sink : sinks) {
                if (mutedSinks.contains(sink->name())) {
                    sink->setMuted(false);
                }
            }
            mutedSinks.clear();
        }

        if (pause && !pausedSources.empty()) {
            for (const QString& iface : qAsConst(pausedSources)) {
                OrgMprisMediaPlayer2PlayerInterface mprisInterface(iface, QStringLiteral("/org/mpris/MediaPlayer2"), DBusHelper::sessionBus());
                mprisInterface.Play();
            }
            pausedSources.clear();
        }

    }

    return true;

}

#include "pausemusicplugin.moc"
