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

#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <qdbusconnectioninterface.h>
#include <QDBusReply>
#include <QDBusMessage>

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< PauseMusicPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_pausemusic", "kdeconnect_pausemusic") )

PauseMusicPlugin::PauseMusicPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{

    //TODO: Be able to change this from plugin settings
    pauseWhen = PauseWhenRinging;

}

bool PauseMusicPlugin::receivePackage(const NetworkPackage& np)
{

    if (np.type() != PACKAGE_TYPE_TELEPHONY) {
        return false;
    }


    if (pauseWhen == PauseWhenRinging) {

        if (np.get<QString>("event") != "ringing" && np.get<QString>("event") != "talking") {
            return true;
        }

    } else if (pauseWhen == PauseWhenTalking) {

        if (np.get<QString>("event") != "talking") {
            return true;
        }

    }


    bool pauseConditionFulfilled = !np.get<bool>("isCancel");

    if (pauseConditionFulfilled) {
        //Search for interfaces currently playing
        QStringList interfaces = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
        Q_FOREACH (const QString& iface, interfaces) {
            if (iface.startsWith("org.mpris.MediaPlayer2")) {
                QDBusInterface mprisInterface(iface, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player");
                QString status = mprisInterface.property("PlaybackStatus").toString();
                if (status == "Playing") {
                    if (!pausedSources.contains(iface)) {
                        pausedSources.insert(iface);
                        mprisInterface.asyncCall("Pause");
                    }
                }
            }
        }
    } else {
        Q_FOREACH (const QString& iface, pausedSources) {
            QDBusInterface mprisInterface(iface, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player");
            //Calling play does not work in spotify
            //mprisInterface->call(QDBus::Block,"Play");
            //Workaround: Using playpause instead (checking first if it is already playing)
            QString status = mprisInterface.property("PlaybackStatus").toString();
            if (status == "Paused") mprisInterface.asyncCall("PlayPause");
            //End of workaround
        }
        pausedSources.clear();
    }

    return true;

}
