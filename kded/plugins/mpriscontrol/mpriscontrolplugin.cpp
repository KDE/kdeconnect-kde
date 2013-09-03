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

#include "mpriscontrolplugin.h"

#include "mprisdbusinterface.h"
#include "propertiesdbusinterface.h"

#include <QDBusArgument>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusInterface>
#include <qdbusconnectioninterface.h>
#include <QDBusReply>
#include <QDBusMessage>

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< MprisControlPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_mpriscontrol", "kdeconnect_mpriscontrol") )

MprisControlPlugin::MprisControlPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
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

void MprisControlPlugin::serviceOwnerChanged(const QString &name,
                                              const QString &oldOwner,
                                              const QString &newOwner)
{
    Q_UNUSED(oldOwner);

    if (name.startsWith("org.mpris.MediaPlayer2")) {

        qDebug() << "Mpris (un)registered in bus" << name << oldOwner << newOwner;

        if (oldOwner.isEmpty()) {
            addPlayer(name);
        } else if (newOwner.isEmpty()) {
            removePlayer(name);
        }
    }
}

void MprisControlPlugin::addPlayer(const QString& service)
{
    QDBusInterface mprisInterface(service, "/org/mpris/MediaPlayer2", "org.mpris.MediaPlayer2");
    //FIXME: This call hangs and returns an empty string if KDED is still starting!
    const QString& identity = mprisInterface.property("Identity").toString();
    playerList[identity] = service;
    qDebug() << "Mpris addPlayer" << service << "->" << identity;
    sendPlayerList();

    OrgFreedesktopDBusPropertiesInterface* freedesktopInterface = new OrgFreedesktopDBusPropertiesInterface(service, "/org/mpris/MediaPlayer2", QDBusConnection::sessionBus(), this);
    connect(freedesktopInterface, SIGNAL(PropertiesChanged(QString, QVariantMap, QStringList)), this, SLOT(propertiesChanged(QString, QVariantMap)));

}

void MprisControlPlugin::propertiesChanged(const QString& propertyInterface, const QVariantMap& properties)
{
    Q_UNUSED(propertyInterface);
    
    NetworkPackage np(PACKAGE_TYPE_MPRIS);
    bool somethingToSend = false;
    if (properties.contains("Volume")) {
        int volume = (int) (properties["Volume"].toDouble()*100);
        if (volume != prevVolume) {
            np.set("volume",volume);
            prevVolume = volume;
            somethingToSend = true;
        }
    }
    if (properties.contains("Metadata")) {
        QDBusArgument bullshit = qvariant_cast<QDBusArgument>(properties["Metadata"]);
        QVariantMap nowPlayingMap;
        bullshit >> nowPlayingMap;
        if (nowPlayingMap.contains("xesam:title")) {
            QString nowPlaying = nowPlayingMap["xesam:title"].toString();
            if (nowPlayingMap.contains("xesam:artist")) {
                nowPlaying = nowPlayingMap["xesam:artist"].toString() + " - " + nowPlaying;
            }
            np.set("nowPlaying",nowPlaying);
            somethingToSend = true;
        }

    }
    if (properties.contains("PlaybackStatus")) {
        bool playing = (properties["PlaybackStatus"].toString() == "Playing");
        np.set("isPlaying", playing);
        somethingToSend = true;
    }

    if (somethingToSend) {
        OrgFreedesktopDBusPropertiesInterface* interface = (OrgFreedesktopDBusPropertiesInterface*)sender();
        const QString& service = interface->service();
        const QString& player = playerList.key(service);
        np.set("player", player);
        device()->sendPackage(np);
    }
}

void MprisControlPlugin::removePlayer(const QString& ifaceName)
{
    QString identity = playerList.key(ifaceName);
    qDebug() << "Mpris removePlayer" << ifaceName << "->" << identity;
    playerList.remove(identity);
    sendPlayerList();
}

bool MprisControlPlugin::receivePackage (const NetworkPackage& np)
{

    if (np.type() != PACKAGE_TYPE_MPRIS) {
        return false;
    }

    if (np.has("playerList")) {
        return false; //Whoever sent this is an mpris client and not an mpris control!
    }

    //Send the player list
    const QString& player = np.get<QString>("player");
    bool valid_player = playerList.contains(player);
    if (!valid_player || np.get<bool>("requestPlayerList")) {
        sendPlayerList();
        if (!valid_player) {
            return true;
        }
    }

    //Do something to the mpris interface
    OrgMprisMediaPlayer2PlayerInterface mprisInterface(playerList[player], "/org/mpris/MediaPlayer2", QDBusConnection::sessionBus());
    if (np.has("action")) {
        const QString& action = np.get<QString>("action");
        qDebug() << "Calling action" << action << "in" << playerList[player];
        //TODO: Check for valid actions
        mprisInterface.call(action);
    }
    if (np.has("setVolume")) {
        double volume = np.get<int>("setVolume")/100.f;
        qDebug() << "Setting volume" << volume << "to" << playerList[player];
        mprisInterface.setVolume(volume);
    }

    //Send something read from the mpris interface
    NetworkPackage answer(PACKAGE_TYPE_MPRIS);
    bool somethingToSend = false;
    if (np.get<bool>("requestNowPlaying")) {

        QVariantMap nowPlayingMap = mprisInterface.metadata();
        QString nowPlaying = nowPlayingMap["xesam:title"].toString();
        if (nowPlayingMap.contains("xesam:artist")) {
            nowPlaying = nowPlayingMap["xesam:artist"].toString() + " - " + nowPlaying;
        }

        answer.set("nowPlaying",nowPlaying);



        bool playing = (mprisInterface.playbackStatus() == QLatin1String("Playing"));
        answer.set("isPlaying", playing);

        somethingToSend = true;


    }
    if (np.get<bool>("requestVolume")) {
        int volume = (int)(mprisInterface.volume() * 100);
        answer.set("volume",volume);
        somethingToSend = true;

    }
    if (somethingToSend) {
        answer.set("player", player);
        device()->sendPackage(answer);
    }

    return true;

}

void MprisControlPlugin::sendPlayerList()
{
    NetworkPackage np(PACKAGE_TYPE_MPRIS);
    np.set("playerList",playerList.keys());
    device()->sendPackage(np);
}
