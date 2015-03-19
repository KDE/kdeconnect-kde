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

#include <KPluginFactory>

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_pausemusic.json", registerPlugin< PauseMusicPlugin >(); )

//TODO: Port this away from KMix to use only Pulseaudio
int PauseMusicPlugin::isKMixMuted() {
    QDBusInterface kmixInterface("org.kde.kmix", "/Mixers", "org.kde.KMix.MixSet");
    QString mixer = kmixInterface.property("currentMasterMixer").toString();
    QString control = kmixInterface.property("currentMasterControl").toString();

    if (mixer.isEmpty() || control.isEmpty())
        return -1;

    mixer.replace(':','_');
    mixer.replace('.','_');
    mixer.replace('-','_');
    control.replace(':','_');
    control.replace('.','_');
    control.replace('-','_');

    QDBusInterface mixerInterface("org.kde.kmix", "/Mixers/"+mixer+"/"+control, "org.kde.KMix.Control");
    if (mixerInterface.property("mute").toBool()) return 1;
    return (mixerInterface.property("volume").toInt() == 0);
}

PauseMusicPlugin::PauseMusicPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , muted(false)
{
    QDBusInterface kmixInterface("org.kde.kmix", "/kmix/KMixWindow/actions/mute", "org.qtproject.Qt.QAction");
}

bool PauseMusicPlugin::receivePackage(const NetworkPackage& np)
{
    bool pauseOnlyWhenTalking = config()->get("conditionTalking", false);

    if (pauseOnlyWhenTalking) {
        if (np.get<QString>("event") != "talking") {
            return true;
        }
    } else { //Pause as soon as it rings
        if (np.get<QString>("event") != "ringing" && np.get<QString>("event") != "talking") {
            return true;
        }
    }

    bool pauseConditionFulfilled = !np.get<bool>("isCancel");

    bool pause = config()->get("actionPause", true);
    bool mute = config()->get("actionMute", false);

    if (pauseConditionFulfilled) {

        if (mute) {
            QDBusInterface kmixInterface("org.kde.kmix", "/kmix/KMixWindow/actions/mute", "org.qtproject.Qt.QAction");
            if (isKMixMuted() == 0) {
                muted = true;
                kmixInterface.call("trigger");
            }
        }

        if (pause) {
            //Search for interfaces currently playing
            QStringList interfaces = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
            Q_FOREACH (const QString& iface, interfaces) {
                if (iface.startsWith("org.mpris.MediaPlayer2")) {
                    QDBusInterface mprisInterface(iface, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player");
                    QString status = mprisInterface.property("PlaybackStatus").toString();
                    if (status == "Playing") {
                        if (!pausedSources.contains(iface)) {
                            pausedSources.insert(iface);
                            if (mprisInterface.property("CanPause").toBool()) {
                                mprisInterface.asyncCall("Pause");
                            } else {
                                mprisInterface.asyncCall("Stop");
                            }
                        }
                    }
                }
            }
        }

    } else {

        if (mute && muted) {
             QDBusInterface kmixInterface("org.kde.kmix", "/kmix/KMixWindow/actions/mute", "org.qtproject.Qt.QAction");
             if (isKMixMuted() > 0) {
                 kmixInterface.call("trigger");
             }
             muted = false;
        }

        if (pause && !pausedSources.empty()) {
            Q_FOREACH (const QString& iface, pausedSources) {
                QDBusInterface mprisInterface(iface, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player");
                //Calling play does not work for Spotify
                //mprisInterface->call(QDBus::Block,"Play");
                //Workaround: Using playpause instead (checking first if it is already playing)
                QString status = mprisInterface.property("PlaybackStatus").toString();
                mprisInterface.asyncCall("PlayPause");
                //End of workaround
            }
            pausedSources.clear();
        }

    }

    return true;

}

#include "pausemusicplugin.moc"
