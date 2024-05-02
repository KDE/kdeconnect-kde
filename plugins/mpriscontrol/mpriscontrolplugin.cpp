/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mpriscontrolplugin.h"

#include <QDBusArgument>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDBusServiceWatcher>
#include <qdbusconnectioninterface.h>

#include <KPluginFactory>

#include <core/device.h>
#include <dbushelper.h>

#include "generated/systeminterfaces/dbusproperties.h"
#include "generated/systeminterfaces/mprisplayer.h"
#include "generated/systeminterfaces/mprisroot.h"
#include "plugin_mpriscontrol_debug.h"

K_PLUGIN_CLASS_WITH_JSON(MprisControlPlugin, "kdeconnect_mpriscontrol.json")

MprisPlayer::MprisPlayer(const QString &serviceName, const QString &dbusObjectPath, const QDBusConnection &busConnection)
    : m_serviceName(serviceName)
    , m_propertiesInterface(new OrgFreedesktopDBusPropertiesInterface(serviceName, dbusObjectPath, busConnection))
    , m_mediaPlayer2PlayerInterface(new OrgMprisMediaPlayer2PlayerInterface(serviceName, dbusObjectPath, busConnection))
{
    m_mediaPlayer2PlayerInterface->setTimeout(500);
}

MprisControlPlugin::MprisControlPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , prevVolume(-1)
{
    m_watcher = new QDBusServiceWatcher(QString(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);

    // TODO: QDBusConnectionInterface::serviceOwnerChanged is deprecated, maybe query org.freedesktop.DBus directly?
    connect(QDBusConnection::sessionBus().interface(), &QDBusConnectionInterface::serviceOwnerChanged, this, &MprisControlPlugin::serviceOwnerChanged);

    // Add existing interfaces
    const QStringList services = QDBusConnection::sessionBus().interface()->registeredServiceNames().value();
    for (const QString &service : services) {
        // The string doesn't matter, it just needs to be empty/non-empty
        serviceOwnerChanged(service, QLatin1String(""), QStringLiteral("1"));
    }
}

// Copied from the mpris2 dataengine in the plasma-workspace repository
void MprisControlPlugin::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)
{
    if (!serviceName.startsWith(QStringLiteral("org.mpris.MediaPlayer2.")))
        return;
    if (serviceName.startsWith(QStringLiteral("org.mpris.MediaPlayer2.kdeconnect.")))
        return;
    // playerctld is a only a proxy to other media players, and can thus safely be ignored
    if (serviceName == QStringLiteral("org.mpris.MediaPlayer2.playerctld"))
        return;

    if (!oldOwner.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_MPRISCONTROL) << "MPRIS service" << serviceName << "just went offline";
        removePlayer(serviceName);
    }

    if (!newOwner.isEmpty()) {
        qCDebug(KDECONNECT_PLUGIN_MPRISCONTROL) << "MPRIS service" << serviceName << "just came online";
        addPlayer(serviceName);
    }
}

void MprisControlPlugin::addPlayer(const QString &service)
{
    const QString mediaPlayerObjectPath = QStringLiteral("/org/mpris/MediaPlayer2");

    OrgMprisMediaPlayer2Interface iface(service, mediaPlayerObjectPath, QDBusConnection::sessionBus());
    QString identity = iface.identity();

    if (identity.isEmpty()) {
        identity = service.mid(sizeof("org.mpris.MediaPlayer2"));
    }

    QString uniqueName = identity;
    for (int i = 2; playerList.contains(uniqueName); ++i) {
        uniqueName = identity + QLatin1String(" [") + QString::number(i) + QLatin1Char(']');
    }

    MprisPlayer player(service, mediaPlayerObjectPath, QDBusConnection::sessionBus());

    playerList.insert(uniqueName, player);

    connect(player.propertiesInterface(), &OrgFreedesktopDBusPropertiesInterface::PropertiesChanged, this, &MprisControlPlugin::propertiesChanged);
    connect(player.mediaPlayer2PlayerInterface(), &OrgMprisMediaPlayer2PlayerInterface::Seeked, this, &MprisControlPlugin::seeked);

    qCDebug(KDECONNECT_PLUGIN_MPRISCONTROL) << "Mpris addPlayer" << service << "->" << uniqueName;
    sendPlayerList();
}

void MprisControlPlugin::seeked(qlonglong position)
{
    // qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Seeked in player";
    OrgMprisMediaPlayer2PlayerInterface *mediaPlayer2PlayerInterface = (OrgMprisMediaPlayer2PlayerInterface *)sender();
    const auto end = playerList.constEnd();
    const auto it = std::find_if(playerList.constBegin(), end, [mediaPlayer2PlayerInterface](const MprisPlayer &player) {
        return (player.mediaPlayer2PlayerInterface() == mediaPlayer2PlayerInterface);
    });
    if (it == end) {
        qCWarning(KDECONNECT_PLUGIN_MPRISCONTROL) << "Seeked signal received for no longer tracked service" << mediaPlayer2PlayerInterface->service();
        return;
    }

    const QString &playerName = it.key();

    NetworkPacket np(PACKET_TYPE_MPRIS,
                     {{QStringLiteral("pos"), position / 1000}, // Send milis instead of nanos
                      {QStringLiteral("player"), playerName}});
    sendPacket(np);
}

void MprisControlPlugin::propertiesChanged(const QString & /*propertyInterface*/, const QVariantMap &properties)
{
    OrgFreedesktopDBusPropertiesInterface *propertiesInterface = (OrgFreedesktopDBusPropertiesInterface *)sender();
    const auto end = playerList.constEnd();
    const auto it = std::find_if(playerList.constBegin(), end, [propertiesInterface](const MprisPlayer &player) {
        return (player.propertiesInterface() == propertiesInterface);
    });
    if (it == end) {
        qCWarning(KDECONNECT_PLUGIN_MPRISCONTROL) << "PropertiesChanged signal received for no longer tracked service" << propertiesInterface->service();
        return;
    }

    OrgMprisMediaPlayer2PlayerInterface *const mediaPlayer2PlayerInterface = it.value().mediaPlayer2PlayerInterface();
    const QString &playerName = it.key();

    NetworkPacket np(PACKET_TYPE_MPRIS);
    bool somethingToSend = false;
    if (properties.contains(QStringLiteral("Volume"))) {
        int volume = (int)(properties[QStringLiteral("Volume")].toDouble() * 100);
        if (volume != prevVolume) {
            np.set(QStringLiteral("volume"), volume);
            prevVolume = volume;
            somethingToSend = true;
        }
    }
    if (properties.contains(QStringLiteral("Metadata"))) {
        QDBusArgument aux = qvariant_cast<QDBusArgument>(properties[QStringLiteral("Metadata")]);
        QVariantMap nowPlayingMap;
        aux >> nowPlayingMap;

        mprisPlayerMetadataToNetworkPacket(np, nowPlayingMap);
        somethingToSend = true;
    }
    if (properties.contains(QStringLiteral("PlaybackStatus"))) {
        bool playing = (properties[QStringLiteral("PlaybackStatus")].toString() == QLatin1String("Playing"));
        np.set(QStringLiteral("isPlaying"), playing);
        somethingToSend = true;
    }
    if (properties.contains(QStringLiteral("LoopStatus"))) {
        np.set(QStringLiteral("loopStatus"), properties[QStringLiteral("LoopStatus")]);
        somethingToSend = true;
    }
    if (properties.contains(QStringLiteral("Shuffle"))) {
        np.set(QStringLiteral("shuffle"), properties[QStringLiteral("Shuffle")].toBool());
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

    if (somethingToSend) {
        np.set(QStringLiteral("player"), playerName);
        // Always also update the position if can seek
        bool canSeek = mediaPlayer2PlayerInterface->canSeek();
        np.set(QStringLiteral("canSeek"), canSeek);
        if (canSeek) {
            long long pos = mediaPlayer2PlayerInterface->position();
            np.set(QStringLiteral("pos"), pos / 1000); // Send milis instead of nanos
        }
        sendPacket(np);
    }
}

void MprisControlPlugin::removePlayer(const QString &serviceName)
{
    const auto end = playerList.end();
    const auto it = std::find_if(playerList.begin(), end, [serviceName](const MprisPlayer &player) {
        return (player.serviceName() == serviceName);
    });
    if (it == end) {
        qCWarning(KDECONNECT_PLUGIN_MPRISCONTROL) << "Could not find player for serviceName" << serviceName;
        return;
    }

    const QString &playerName = it.key();
    qCDebug(KDECONNECT_PLUGIN_MPRISCONTROL) << "Mpris removePlayer" << serviceName << "->" << playerName;

    playerList.erase(it);

    sendPlayerList();
}

bool MprisControlPlugin::sendAlbumArt(const NetworkPacket &np)
{
    const QString player = np.get<QString>(QStringLiteral("player"));
    auto it = playerList.find(player);
    bool valid_player = (it != playerList.end());
    if (!valid_player) {
        return false;
    }

    // Get mpris information
    auto &mprisInterface = *it.value().mediaPlayer2PlayerInterface();
    QVariantMap nowPlayingMap = mprisInterface.metadata();

    // Check if the supplied album art url indeed belongs to this mpris player
    QUrl playerAlbumArtUrl{nowPlayingMap[QStringLiteral("mpris:artUrl")].toString()};
    QString requestedAlbumArtUrl = np.get<QString>(QStringLiteral("albumArtUrl"));
    if (!playerAlbumArtUrl.isValid() || playerAlbumArtUrl != QUrl(requestedAlbumArtUrl)) {
        return false;
    }

    // Only support sending local files
    if (playerAlbumArtUrl.scheme() != QStringLiteral("file")) {
        return false;
    }

    // Open the file to send
    QSharedPointer<QFile> art{new QFile(playerAlbumArtUrl.toLocalFile())};

    // Send the album art as payload
    NetworkPacket answer(PACKET_TYPE_MPRIS);
    answer.set(QStringLiteral("transferringAlbumArt"), true);
    answer.set(QStringLiteral("player"), player);
    answer.set(QStringLiteral("albumArtUrl"), requestedAlbumArtUrl);
    answer.setPayload(art, art->size());
    sendPacket(answer);
    return true;
}

void MprisControlPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.has(QStringLiteral("playerList"))) {
        return; // Whoever sent this is an mpris client and not an mpris control!
    }

    if (np.has(QStringLiteral("albumArtUrl"))) {
        sendAlbumArt(np);
        return;
    }

    // Send the player list
    const QString player = np.get<QString>(QStringLiteral("player"));
    auto it = playerList.find(player);
    bool valid_player = (it != playerList.end());
    if (!valid_player || np.get<bool>(QStringLiteral("requestPlayerList"))) {
        sendPlayerList();
        if (!valid_player) {
            return;
        }
    }

    // Do something to the mpris interface
    const QString &serviceName = it.value().serviceName();
    // turn from pointer to reference to keep the patch diff small,
    // actual patch would change all "mprisInterface." into "mprisInterface->"
    auto &mprisInterface = *it.value().mediaPlayer2PlayerInterface();
    if (np.has(QStringLiteral("action"))) {
        const QString &action = np.get<QString>(QStringLiteral("action"));
        // qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Calling action" << action << "in" << serviceName;
        // TODO: Check for valid actions, currently we trust anything the other end sends us
        mprisInterface.call(action);
    }
    if (np.has(QStringLiteral("setLoopStatus"))) {
        const QString &loopStatus = np.get<QString>(QStringLiteral("setLoopStatus"));
        qCDebug(KDECONNECT_PLUGIN_MPRISCONTROL) << "Setting loopStatus" << loopStatus << "to" << serviceName;
        mprisInterface.setLoopStatus(loopStatus);
    }
    if (np.has(QStringLiteral("setShuffle"))) {
        bool shuffle = np.get<bool>(QStringLiteral("setShuffle"));
        qCDebug(KDECONNECT_PLUGIN_MPRISCONTROL) << "Setting shuffle" << shuffle << "to" << serviceName;
        mprisInterface.setShuffle(shuffle);
    }
    if (np.has(QStringLiteral("setVolume"))) {
        double volume = np.get<int>(QStringLiteral("setVolume")) / 100.f;
        qCDebug(KDECONNECT_PLUGIN_MPRISCONTROL) << "Setting volume" << volume << "to" << serviceName;
        mprisInterface.setVolume(volume);
    }
    if (np.has(QStringLiteral("Seek"))) {
        int offset = np.get<int>(QStringLiteral("Seek"));
        // qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Seeking" << offset << "to" << serviceName;
        mprisInterface.Seek(offset);
    }

    if (np.has(QStringLiteral("SetPosition"))) {
        qlonglong position = np.get<qlonglong>(QStringLiteral("SetPosition"), 0) * 1000;
        qlonglong seek = position - mprisInterface.position();
        // qCDebug(KDECONNECT_PLUGIN_MPRIS) << "Setting position by seeking" << seek << "to" << serviceName;
        mprisInterface.Seek(seek);
    }

    // Send something read from the mpris interface
    NetworkPacket answer(PACKET_TYPE_MPRIS);
    bool somethingToSend = false;
    if (np.get<bool>(QStringLiteral("requestNowPlaying"))) {
        QVariantMap nowPlayingMap = mprisInterface.metadata();
        mprisPlayerMetadataToNetworkPacket(answer, nowPlayingMap);

        qlonglong pos = mprisInterface.position();
        answer.set(QStringLiteral("pos"), pos / 1000);

        bool playing = (mprisInterface.playbackStatus() == QLatin1String("Playing"));
        answer.set(QStringLiteral("isPlaying"), playing);

        answer.set(QStringLiteral("canPause"), mprisInterface.canPause());
        answer.set(QStringLiteral("canPlay"), mprisInterface.canPlay());
        answer.set(QStringLiteral("canGoNext"), mprisInterface.canGoNext());
        answer.set(QStringLiteral("canGoPrevious"), mprisInterface.canGoPrevious());
        answer.set(QStringLiteral("canSeek"), mprisInterface.canSeek());

        // LoopStatus is an optional field
        if (mprisInterface.property("LoopStatus").isValid()) {
            const QString &loopStatus = mprisInterface.loopStatus();
            answer.set(QStringLiteral("loopStatus"), loopStatus);
        }

        // Shuffle is an optional field
        if (mprisInterface.property("Shuffle").isValid()) {
            bool shuffle = mprisInterface.shuffle();
            answer.set(QStringLiteral("shuffle"), shuffle);
        }

        somethingToSend = true;
    }
    if (np.get<bool>(QStringLiteral("requestVolume"))) {
        int volume = (int)(mprisInterface.volume() * 100);
        answer.set(QStringLiteral("volume"), volume);
        somethingToSend = true;
    }

    if (somethingToSend) {
        answer.set(QStringLiteral("player"), player);
        sendPacket(answer);
    }
}

QStringList getFilteredPlayerKeys(const QHash<QString, MprisPlayer> &players)
{
    bool hasPlasmaChromiumExtension = false;
    bool hasPlasmaFirefoxExtension = false;
    for (auto [name, player] : players.asKeyValueRange()) {
        QString serviceName = player.serviceName();
        if (serviceName.startsWith(QStringLiteral("org.mpris.MediaPlayer2.plasma-browser-integration"))) {
            if (name.contains(QStringLiteral("Firefox"))) {
                hasPlasmaFirefoxExtension = true;
            } else if (name.contains(QStringLiteral("Chrome")) || name.contains(QStringLiteral("Chromium"))) {
                hasPlasmaChromiumExtension = true;
            }
        }
    }
    QStringList result;
    for (auto [name, player] : players.asKeyValueRange()) {
        QString serviceName = player.serviceName();
        if ((hasPlasmaFirefoxExtension && serviceName.startsWith(QStringLiteral("org.mpris.MediaPlayer2.firefox")))
            || (hasPlasmaChromiumExtension && serviceName.startsWith(QStringLiteral("org.mpris.MediaPlayer2.chromium")))) {
            continue; // skip
        }
        result += name;
    }
    return result;
}

void MprisControlPlugin::sendPlayerList()
{
    QStringList players = getFilteredPlayerKeys(playerList);
    NetworkPacket np(PACKET_TYPE_MPRIS);
    np.set(QStringLiteral("playerList"), players);
    np.set(QStringLiteral("supportAlbumArtPayload"), true);
    sendPacket(np);
}

void MprisControlPlugin::mprisPlayerMetadataToNetworkPacket(NetworkPacket &np, const QVariantMap &nowPlayingMap) const
{
    QString title = nowPlayingMap[QStringLiteral("xesam:title")].toString();
    QString artist = nowPlayingMap[QStringLiteral("xesam:artist")].toStringList().join(QLatin1String(", "));
    QString album = nowPlayingMap[QStringLiteral("xesam:album")].toString();
    QString albumArtUrl = nowPlayingMap[QStringLiteral("mpris:artUrl")].toString();
    QUrl fileUrl = nowPlayingMap[QStringLiteral("xesam:url")].toUrl();

    if (title.isEmpty() && artist.isEmpty() && fileUrl.isLocalFile()) {
        title = fileUrl.fileName();

        QStringList splitUrl = fileUrl.path().split(QDir::separator());
        if (album.isEmpty() && splitUrl.size() > 1) {
            album = splitUrl.at(splitUrl.size() - 2);
        }
    }

    np.set(QStringLiteral("title"), title);
    np.set(QStringLiteral("artist"), artist);
    np.set(QStringLiteral("album"), album);
    np.set(QStringLiteral("albumArtUrl"), albumArtUrl);

    bool hasLength = false;
    long long length = nowPlayingMap[QStringLiteral("mpris:length")].toLongLong(&hasLength) / 1000; // nanoseconds to milliseconds
    if (!hasLength) {
        length = -1;
    }
    np.set(QStringLiteral("length"), length);
    np.set(QStringLiteral("url"), fileUrl);
}

#include "moc_mpriscontrolplugin.cpp"
#include "mpriscontrolplugin.moc"
