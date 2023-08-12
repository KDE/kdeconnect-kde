/**
 * SPDX-FileCopyrightText: 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
 * SPDX-FileCopyrightText: 2019 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "findthisdeviceplugin.h"

// KF
#include <KPluginFactory>

#ifndef Q_OS_WIN
#include <PulseAudioQt/Context>
#include <PulseAudioQt/Sink>
#endif

// Qt
#include "plugin_findthisdevice_debug.h"
#include <QDBusConnection>
#include <QMediaPlayer>

#if QT_VERSION_MAJOR == 6
#include <QAudioOutput>
#endif

K_PLUGIN_CLASS_WITH_JSON(FindThisDevicePlugin, "kdeconnect_findthisdevice.json")

void FindThisDevicePlugin::receivePacket(const NetworkPacket & /*np*/)
{
    const QString soundFile = config()->getString(QStringLiteral("ringtone"), defaultSound());
    const QUrl soundURL = QUrl::fromLocalFile(soundFile);

    if (soundURL.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_FINDTHISDEVICE) << "Not playing sound, no valid ring tone specified.";
        return;
    }

    QMediaPlayer *player = new QMediaPlayer;
#if QT_VERSION_MAJOR < 6
    player->setAudioRole(QAudio::Role(QAudio::NotificationRole));
    player->setMedia(soundURL);
    player->setVolume(100);
#else
    auto audioOutput = new QAudioOutput();
    audioOutput->setVolume(100);
    player->setSource(soundURL);
    player->setAudioOutput(audioOutput);
    connect(player, &QMediaPlayer::playingChanged, player, &QObject::deleteLater);
#endif
    player->play();

#ifndef Q_OS_WIN
    const auto sinks = PulseAudioQt::Context::instance()->sinks();
    QVector<PulseAudioQt::Sink *> mutedSinks;
    for (auto sink : sinks) {
        if (sink->isMuted()) {
            sink->setMuted(false);
            mutedSinks.append(sink);
        }
    }
#if QT_VERSION_MAJOR < 6
    connect(player, &QMediaPlayer::stateChanged, this, [mutedSinks] {
#else
    connect(player, &QMediaPlayer::playingChanged, this, [mutedSinks] {
#endif
        for (auto sink : qAsConst(mutedSinks)) {
            sink->setMuted(true);
        }
    });
#endif

    player->play();
#if QT_VERSION_MAJOR < 6
    connect(player, &QMediaPlayer::stateChanged, player, &QObject::deleteLater);
#else
    connect(player, &QMediaPlayer::playingChanged, player, &QObject::deleteLater);
#endif
    // TODO: ensure to use built-in loudspeakers
}

QString FindThisDevicePlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/findthisdevice").arg(device()->id());
}

#include "findthisdeviceplugin.moc"
#include "moc_findthisdeviceplugin.cpp"
