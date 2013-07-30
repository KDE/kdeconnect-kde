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

#include "mpriscontrolpackageinterface.h"

#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <qdbusconnectioninterface.h>
#include <QDBusReply>
#include <QDBusMessage>

MprisControlPackageInterface::MprisControlPackageInterface()
{

    //Detect new interfaces
    connect(QDBusConnection::sessionBus().interface(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));

    //Add existing interfaces
    QStringList interfaces = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
    Q_FOREACH (const QString& iface, interfaces) {
        if (iface.startsWith("org.mpris.MediaPlayer2")) {
            addPlayer(iface);
        }
    }

}

void MprisControlPackageInterface::serviceOwnerChanged(const QString &name,
                                              const QString &oldOwner,
                                              const QString &newOwner)
{
    Q_UNUSED(oldOwner);

    if (name.startsWith("org.mpris.MediaPlayer2")) {

        qDebug() << "Something registered in bus" << name << oldOwner << newOwner;

        if (oldOwner.isEmpty()) {
            addPlayer(name);
        } else if (newOwner.isEmpty()) {
            removePlayer(name);
        }
    }
}

void MprisControlPackageInterface::addPlayer(const QString& ifaceName)
{
    QDBusInterface interface(ifaceName, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2");
    //TODO: Make this async
    QString identity = interface.property("Identity").toString();
    playerList[identity] = ifaceName;
    qDebug() << "addPlayer" << ifaceName << identity;
    sendPlayerList();
}

void MprisControlPackageInterface::removePlayer(const QString& ifaceName)
{
    playerList.remove(playerList.key(ifaceName));
    sendPlayerList();
}

void MprisControlPackageInterface::sendPlayerList()
{
    NetworkPackage np(PACKAGE_TYPE_MPRIS);
    np.set("playerList",playerList.keys());
    sendPackage(np);
}

bool MprisControlPackageInterface::receivePackage (const Device& device, const NetworkPackage& np)
{
    Q_UNUSED(device);

    if (np.type() != PACKAGE_TYPE_MPRIS) return false;


    QString player = np.get<QString>("player");
    if (!playerList.contains(player)) {
        sendPlayerList();
        return true;
    }

    QDBusInterface mprisInterface(playerList[player], "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2.Player");

    if (np.get<bool>("requestPlayerList")) {
        sendPlayerList();
    }

    QString action = np.get<QString>("action");
    if (!action.isEmpty()) {
        qDebug() << "Calling action" << action << "in" << playerList[player];
        mprisInterface.call(action);
        sendNowPlaying(mprisInterface);
    } else if (np.get<bool>("requestNowPlaying")) {
        sendNowPlaying(mprisInterface);
    }

    return true;

}

void MprisControlPackageInterface::sendNowPlaying(const QDBusInterface& mprisInterface)
{
    QVariantMap nowPlayingMap = mprisInterface.property("Metadata").toMap();
    QString nowPlaying = nowPlayingMap["xesam:title"].toString();
    if (nowPlayingMap.contains("xesam:artist")) {
        nowPlaying = nowPlayingMap["xesam:artist"].toString() + " - " + nowPlaying;
    }
    NetworkPackage np(PACKAGE_TYPE_MPRIS);
    np.set("nowPlaying",nowPlaying);
    sendPackage(np);
}
