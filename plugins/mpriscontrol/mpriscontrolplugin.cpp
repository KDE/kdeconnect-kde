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

#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusInterface>
#include <qdbusconnectioninterface.h>
#include <QDBusReply>
#include <QDBusMessage>
#include <QDBusServiceWatcher>

#include <KPluginFactory>

#include <core/device.h>
#include "mprisdbusinterface.h"
#include "propertiesdbusinterface.h"

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_mpriscontrol.json", registerPlugin< MprisControlPlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_MPRIS, "kdeconnect.plugin.mpris")

MprisControlPlugin::MprisControlPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
    , prevVolume(-1)
{
    m_watcher = new QDBusServiceWatcher(QString(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);

    // TODO: QDBusConnectionInterface::serviceOwnerChanged is deprecated, maybe query org.freedesktop.DBus directly?
    connect(QDBusConnection::sessionBus().interface(), &QDBusConnectionInterface::serviceOwnerChanged, this, &MprisControlPlugin::serviceOwnerChanged);

    //Add existing interfaces
    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
    for (const QString& service : services) {
        // The string doesn't matter, it just needs to be empty/non-empty
        serviceOwnerChanged(service, QLatin1String(""), QStringLiteral("1"));
    }
}

// Copied from the mpris2 dataengine in the plasma-workspace repository
void MprisControlPlugin::serviceOwnerChanged(const QString& serviceName, const QString& oldOwner, const QString& newOwner)
{
    if (!serviceName.startsWith(QLatin1String("org.mpris.MediaPlayer2.")))
        return;

    if (!oldOwner.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_MPRIS) << "MPRIS service" << serviceName << "just went offline";
        removePlayer(serviceName);
    }

    if (!newOwner.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_MPRIS) << "MPRIS service" << serviceName << "just came online";
        addPlayer(serviceName);
    }
}


void MprisControlPlugin::addPlayer(const QString& service)
{
    QDBusInterface mprisInterface(service, QStringLiteral("/org/mpris/MediaPlayer2"), QStringLiteral("org.mpris.MediaPlayer2"));
    //FIXME: This call hangs and returns an empty string if KDED is still starting!
    QString identity = mprisInterface.property("Identity").toString();
    if (identity.isEmpty()) {
        identity = service.mid(sizeof("org.mpris.MediaPlayer2"));
    }

    QString uniqueName = identity;
    for (int i = 1 ; !playerList[uniqueName].isEmpty() ; i++) {
        uniqueName = identity + " [" + i + "]";
    }

    playerList[uniqueName] = service;
    qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Mpris addPlayer" << service << "->" << uniqueName;
    sendPlayerList();

    OrgFreedesktopDBusPropertiesInterface* freedesktopInterface = new OrgFreedesktopDBusPropertiesInterface(service, QStringLiteral("/org/mpris/MediaPlayer2"), QDBusConnection::sessionBus(), this);
    connect(freedesktopInterface, &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged, this, &MprisControlPlugin::propertiesChanged);

    OrgMprisMediaPlayer2PlayerInterface* mprisInterface0  = new OrgMprisMediaPlayer2PlayerInterface(service, QStringLiteral("/org/mpris/MediaPlayer2"), QDBusConnection::sessionBus());
    connect(mprisInterface0, &OrgMprisMediaPlayer2PlayerInterface::Seeked, this, &MprisControlPlugin::seeked);
}

void MprisControlPlugin::seeked(qlonglong position){
    //qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Seeked in player";
    OrgFreedesktopDBusPropertiesInterface* interface = (OrgFreedesktopDBusPropertiesInterface*)sender();
    const QString& service = interface->service();
    const QString& player = playerList.key(service);

    NetworkPackage np(PACKAGE_TYPE_MPRIS, {
        {"pos", position/1000}, //Send milis instead of nanos
        {"player", player}
    });
    sendPackage(np);
}

void MprisControlPlugin::propertiesChanged(const QString& propertyInterface, const QVariantMap& properties)
{
    Q_UNUSED(propertyInterface);

    NetworkPackage np(PACKAGE_TYPE_MPRIS);
    bool somethingToSend = false;
    if (properties.contains(QStringLiteral("Volume"))) {
        int volume = (int) (properties[QStringLiteral("Volume")].toDouble()*100);
        if (volume != prevVolume) {
            np.set(QStringLiteral("volume"),volume);
            prevVolume = volume;
            somethingToSend = true;
        }
    }
    if (properties.contains(QStringLiteral("Metadata"))) {
        QDBusArgument bullshit = qvariant_cast<QDBusArgument>(properties[QStringLiteral("Metadata")]);
        QVariantMap nowPlayingMap;
        bullshit >> nowPlayingMap;

        mprisPlayerMetadataToNetworkPackage(np, nowPlayingMap);
        somethingToSend = true;
    }
    if (properties.contains(QStringLiteral("PlaybackStatus"))) {
        bool playing = (properties[QStringLiteral("PlaybackStatus")].toString() == QLatin1String("Playing"));
        np.set(QStringLiteral("isPlaying"), playing);
        somethingToSend = true;
    }
    if (properties.contains(QStringLiteral("CanPause"))) {
        np.set(QStringLiteral("canPause"), properties[QStringLiteral("CanPause")].toBool());
        somethingToSend = true;
    }
    if (properties.contains(QStringLiteral("CanPlay"))) {
        np.set(QStringLiteral("canPlay"), properties[QStringLiteral("CanPlay")].toBool());
        somethingToSend = true;
    }
    if (properties.contains(QStringLiteral("CanGoNext"))) {
        np.set(QStringLiteral("canGoNext"), properties[QStringLiteral("CanGoNext")].toBool());
        somethingToSend = true;
    }
    if (properties.contains(QStringLiteral("CanGoPrevious"))) {
        np.set(QStringLiteral("canGoPrevious"), properties[QStringLiteral("CanGoPrevious")].toBool());
        somethingToSend = true;
    }
    if (properties.contains(QStringLiteral("CanSeek"))) {
        np.set(QStringLiteral("canSeek"), properties[QStringLiteral("CanSeek")].toBool());
        somethingToSend = true;
    }

    if (somethingToSend) {
        OrgFreedesktopDBusPropertiesInterface* interface = (OrgFreedesktopDBusPropertiesInterface*)sender();
        const QString& service = interface->service();
        const QString& player = playerList.key(service);
        np.set(QStringLiteral("player"), player);
        // Always also update the position
        OrgMprisMediaPlayer2PlayerInterface mprisInterface(service, QStringLiteral("/org/mpris/MediaPlayer2"), QDBusConnection::sessionBus());
        if (mprisInterface.canSeek()) {
            long long pos = mprisInterface.position();
            np.set(QStringLiteral("pos"), pos/1000); //Send milis instead of nanos
        }
        sendPackage(np);
    }
}

void MprisControlPlugin::removePlayer(const QString& ifaceName)
{
    const QString identity = playerList.key(ifaceName);
    qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Mpris removePlayer" << ifaceName << "->" << identity;
    playerList.remove(identity);
    sendPlayerList();
}

bool MprisControlPlugin::receivePackage (const NetworkPackage& np)
{
    if (np.has(QStringLiteral("playerList"))) {
        return false; //Whoever sent this is an mpris client and not an mpris control!
    }

    //Send the player list
    const QString player = np.get<QString>(QStringLiteral("player"));
    bool valid_player = playerList.contains(player);
    if (!valid_player || np.get<bool>(QStringLiteral("requestPlayerList"))) {
        sendPlayerList();
        if (!valid_player) {
            return true;
        }
    }

    //Do something to the mpris interface
    OrgMprisMediaPlayer2PlayerInterface mprisInterface(playerList[player], QStringLiteral("/org/mpris/MediaPlayer2"), QDBusConnection::sessionBus());
    mprisInterface.setTimeout(500);
    if (np.has(QStringLiteral("action"))) {
        const QString& action = np.get<QString>(QStringLiteral("action"));
        //qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Calling action" << action << "in" << playerList[player];
        //TODO: Check for valid actions, currently we trust anything the other end sends us
        mprisInterface.call(action);
    }
    if (np.has(QStringLiteral("setVolume"))) {
        double volume = np.get<int>(QStringLiteral("setVolume"))/100.f;
        qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Setting volume" << volume << "to" << playerList[player];
        mprisInterface.setVolume(volume);
    }
    if (np.has(QStringLiteral("Seek"))) {
        int offset = np.get<int>(QStringLiteral("Seek"));
        //qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Seeking" << offset << "to" << playerList[player];
        mprisInterface.Seek(offset);
    }

    if (np.has(QStringLiteral("SetPosition"))){
        qlonglong position = np.get<qlonglong>(QStringLiteral("SetPosition"),0)*1000;
        qlonglong seek = position - mprisInterface.position();
        //qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Setting position by seeking" << seek << "to" << playerList[player];
        mprisInterface.Seek(seek);
    }

    //Send something read from the mpris interface
    NetworkPackage answer(PACKAGE_TYPE_MPRIS);
    bool somethingToSend = false;
    if (np.get<bool>(QStringLiteral("requestNowPlaying"))) {
        QVariantMap nowPlayingMap = mprisInterface.metadata();
        mprisPlayerMetadataToNetworkPackage(answer, nowPlayingMap);

        qlonglong pos = mprisInterface.position();
        answer.set(QStringLiteral("pos"), pos/1000);

        bool playing = (mprisInterface.playbackStatus() == QLatin1String("Playing"));
        answer.set(QStringLiteral("isPlaying"), playing);

        answer.set(QStringLiteral("canPause"), mprisInterface.canPause());
        answer.set(QStringLiteral("canPlay"), mprisInterface.canPlay());
        answer.set(QStringLiteral("canGoNext"), mprisInterface.canGoNext());
        answer.set(QStringLiteral("canGoPrevious"), mprisInterface.canGoPrevious());
        answer.set(QStringLiteral("canSeek"), mprisInterface.canSeek());

        somethingToSend = true;
    }
    if (np.get<bool>(QStringLiteral("requestVolume"))) {
        int volume = (int)(mprisInterface.volume() * 100);
        answer.set(QStringLiteral("volume"),volume);
        somethingToSend = true;
    }
    if (somethingToSend) {
        answer.set(QStringLiteral("player"), player);
        sendPackage(answer);
    }

    return true;
}

void MprisControlPlugin::sendPlayerList()
{
    NetworkPackage np(PACKAGE_TYPE_MPRIS);
    np.set(QStringLiteral("playerList"),playerList.keys());
    sendPackage(np);
}

void MprisControlPlugin::mprisPlayerMetadataToNetworkPackage(NetworkPackage& np, const QVariantMap& nowPlayingMap) const {
    QString title = nowPlayingMap[QStringLiteral("xesam:title")].toString();
    QString artist = nowPlayingMap[QStringLiteral("xesam:artist")].toString();
    QString album = nowPlayingMap[QStringLiteral("xesam:album")].toString();
    QString albumArtUrl = nowPlayingMap[QStringLiteral("mpris:artUrl")].toString();
    QString nowPlaying = title;
    if (!artist.isEmpty()) {
        nowPlaying = artist + " - " + title;
    }
    np.set(QStringLiteral("title"), title);
    np.set(QStringLiteral("artist"), artist);
    np.set(QStringLiteral("album"), album);
    np.set(QStringLiteral("albumArtUrl"), albumArtUrl);
    np.set(QStringLiteral("nowPlaying"), nowPlaying);

    bool hasLength = false;
    long long length = nowPlayingMap[QStringLiteral("mpris:length")].toLongLong(&hasLength) / 1000; //nanoseconds to milliseconds
    if (!hasLength) {
        length = -1;
    }
    np.set(QStringLiteral("length"), length);
}

#include "mpriscontrolplugin.moc"
