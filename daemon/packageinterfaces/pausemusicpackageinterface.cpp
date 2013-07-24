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

#include "pausemusicpackageinterface.h"

#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <qdbusconnectioninterface.h>
#include <QDBusReply>
#include <QDBusMessage>

PauseMusicPackageInterface::PauseMusicPackageInterface()
{
    //TODO: Be able to change this from settings
    pauseWhen = PauseWhenRinging;

}

bool PauseMusicPackageInterface::receivePackage (const Device& device, const NetworkPackage& np)
{
    Q_UNUSED(device);

    bool pauseConditionFulfilled = false;

    //TODO: I have manually tested it and it works for both "pauseWhen" cases, but I should somehow write a test for this logic
    if (pauseWhen == PauseWhenRinging) {
        if (np.type() == PACKAGE_TYPE_NOTIFICATION) {
            if (np.get<QString>("notificationType") != "ringing") return false;
            pauseConditionFulfilled = !np.get<bool>("isCancel");
        } else if (np.type() == PACKAGE_TYPE_CALL) {
            pauseConditionFulfilled = !np.get<bool>("isCancel");
        } else {
            return false;
        }
    } else if (pauseWhen == PauseWhenTalking){
        if (np.type() != PACKAGE_TYPE_CALL) return false;
        pauseConditionFulfilled = !np.get<bool>("isCancel");
    }

    qDebug() << "PauseMusicPackageReceiver - PauseCondition:" << pauseConditionFulfilled;

    if (pauseConditionFulfilled) {
        //TODO: Make this async
        //Search for interfaces currently playing
        QStringList interfaces = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
        Q_FOREACH (const QString& iface, interfaces) {
            if (iface.startsWith("org.mpris.MediaPlayer2")) {
                QDBusInterface *dbusInterface = new QDBusInterface(iface, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus(), this);
                QDBusInterface *mprisInterface = new QDBusInterface(iface, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", QDBusConnection::sessionBus(), this);

                QString status = (qvariant_cast<QDBusVariant>(dbusInterface->call(QDBus::Block,"Get","org.mpris.MediaPlayer2.Player","PlaybackStatus").arguments().first()).variant()).toString();
                if (status == "Playing") {
                    if (!pausedSources.contains(iface)) {
                        pausedSources.insert(iface);
                        mprisInterface->call(QDBus::Block,"Pause");
                    }
                }

                delete dbusInterface;
                delete mprisInterface;
            }
        }
    } if (!pauseConditionFulfilled) {
        //TODO: Make this async
        Q_FOREACH (const QString& iface, pausedSources) {
            QDBusInterface *mprisInterface = new QDBusInterface(iface, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player", QDBusConnection::sessionBus(), this);
            //FIXME: Calling play does not work in spotify
            //mprisInterface->call(QDBus::Block,"Play");
            //Workaround: Using playpause instead (checking first if it is already playing)
            QDBusInterface *dbusInterface = new QDBusInterface(iface, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", QDBusConnection::sessionBus(), this);
            QString status = (qvariant_cast<QDBusVariant>(dbusInterface->call(QDBus::Block,"Get","org.mpris.MediaPlayer2.Player","PlaybackStatus").arguments().first()).variant()).toString();
            if (status == "Paused") mprisInterface->call(QDBus::Block,"PlayPause");
            delete dbusInterface;
            //End of workaround
            delete mprisInterface;
        }
        pausedSources.clear();
    }

    return true;

}
