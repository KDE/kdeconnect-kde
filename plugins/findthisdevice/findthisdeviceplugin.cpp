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
#include <QDBusConnection>
#include <QMediaPlayer>
#include "plugin_findthisdevice_debug.h"

K_PLUGIN_CLASS_WITH_JSON(FindThisDevicePlugin, "kdeconnect_findthisdevice.json")

FindThisDevicePlugin::FindThisDevicePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

FindThisDevicePlugin::~FindThisDevicePlugin() = default;


bool FindThisDevicePlugin::receivePacket(const NetworkPacket& np)
{
    Q_UNUSED(np);

    const QString soundFile = config()->getString(QStringLiteral("ringtone"), defaultSound());
    const QUrl soundURL = QUrl::fromLocalFile(soundFile);

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

