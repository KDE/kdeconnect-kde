/**
 * Copyright 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
 * Copyright 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
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

#include "findthisdeviceplugin.h"

// KF
#include <KPluginFactory>

#ifndef Q_OS_WIN
#include <PulseAudioQt/Context>
#include <PulseAudioQt/Sink>
#endif

// Qt
#include <QDBusConnection>
#include <QMediaPlayer>

K_PLUGIN_CLASS_WITH_JSON(FindThisDevicePlugin, "kdeconnect_findthisdevice.json")

FindThisDevicePlugin::FindThisDevicePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

FindThisDevicePlugin::~FindThisDevicePlugin() = default;


bool FindThisDevicePlugin::receivePacket(const NetworkPacket& np)
{
    Q_UNUSED(np);
    const QString soundFile = config()->get<QString>(QStringLiteral("ringtone"), defaultSound());
    const QUrl soundURL = QUrl(soundFile);
    if (soundURL.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_FINDTHISDEVICE) << "Not playing sound, no valid ring tone specified.";
        return true;
    }

    QMediaPlayer* player = new QMediaPlayer;
    player->setAudioRole(QAudio::Role(QAudio::NotificationRole));
    player->setMedia(soundURL);
    player->setVolume(100);

#ifndef Q_OS_WIN
    const auto sinks = PulseAudioQt::Context::instance()->sinks();
    QVector<PulseAudioQt::Sink*> mutedSinks;
    for (auto sink : sinks) {
        if (sink->isMuted()) {
            sink->setMuted(false);
            mutedSinks.append(sink);
        }
    }
    connect(player, &QMediaPlayer::stateChanged, this, [player, mutedSinks]{
        for (auto sink : qAsConst(mutedSinks)) {
            sink->setMuted(true);
        }
    });
#endif

    player->play();
    connect(player, &QMediaPlayer::stateChanged, player, &QObject::deleteLater);
    // TODO: ensure to use built-in loudspeakers

    return true;
}

QString FindThisDevicePlugin::dbusPath() const
{
    return QStringLiteral("/modules/kdeconnect/devices/") + device()->id() + QStringLiteral("/findthisdevice");
}

#include "findthisdeviceplugin.moc"

