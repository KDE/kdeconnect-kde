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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "pausemusicplugin.h"

#include <QDBusConnection>
#include <QDBusInterface>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDebug>
#include <QProcess>

#include <KPluginFactory>

//In older Qt released, qAsConst isnt available
#include "qtcompat_p.h"

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_pausemusic.json", registerPlugin< PauseMusicPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_PAUSEMUSIC, "kdeconnect.plugin.pausemusic")

PauseMusicPlugin::PauseMusicPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , muted(false)
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
            QProcess::startDetached("pactl set-sink-mute @DEFAULT_SINK@ 1");
            muted = true;
        }

        if (pause) {
            //Search for interfaces currently playing
            const QStringList interfaces = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
            for (const QString& iface : interfaces) {
                if (iface.startsWith(QLatin1String("org.mpris.MediaPlayer2"))) {
                    QDBusInterface mprisInterface(iface, QStringLiteral("/org/mpris/MediaPlayer2"), QStringLiteral("org.mpris.MediaPlayer2.Player"));
                    QString status = mprisInterface.property("PlaybackStatus").toString();
                    if (status == QLatin1String("Playing")) {
                        if (!pausedSources.contains(iface)) {
                            pausedSources.insert(iface);
                            if (mprisInterface.property("CanPause").toBool()) {
                                mprisInterface.asyncCall(QStringLiteral("Pause"));
                            } else {
                                mprisInterface.asyncCall(QStringLiteral("Stop"));
                            }
                        }
                    }
                }
            }
        }

    } else {

        if (mute && muted) {

            qCDebug(KDECONNECT_PLUGIN_PAUSEMUSIC) << "Unmuting system volume";
            QProcess::startDetached("pactl set-sink-mute @DEFAULT_SINK@ 0");

            muted = false;
        }

        if (pause && !pausedSources.empty()) {
            for (const QString& iface : qAsConst(pausedSources)) {
                QDBusInterface mprisInterface(iface, QStringLiteral("/org/mpris/MediaPlayer2"), QStringLiteral("org.mpris.MediaPlayer2.Player"));
                mprisInterface.asyncCall(QStringLiteral("PlayPause"));
            }
            pausedSources.clear();
        }

    }

    return true;

}

#include "pausemusicplugin.moc"
