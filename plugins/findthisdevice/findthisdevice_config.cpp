/**
 * SPDX-FileCopyrightText: 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "findthisdevice_config.h"
#include "findthisdeviceplugin.h"

// KF
#include <KLocalizedString>
#include <KPluginFactory>
// Qt
#include <QMediaPlayer>
#include <QStandardPaths>

#include <QAudioOutput>

K_PLUGIN_CLASS(FindThisDeviceConfig)

FindThisDeviceConfig::FindThisDeviceConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KdeConnectPluginKcm(parent, data, args)
{
    m_ui.setupUi(widget());

    const QStringList soundDirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("sounds"), QStandardPaths::LocateDirectory);
    if (!soundDirs.isEmpty()) {
        m_ui.soundFileRequester->setStartDir(QUrl::fromLocalFile(soundDirs.last()));
    }

    connect(m_ui.playSoundButton, &QToolButton::clicked, this, [this]() {
        if (const QUrl soundUrl = m_ui.soundFileRequester->url(); soundUrl.isValid()) {
            playSound(soundUrl);
        }
    });
    connect(m_ui.soundFileRequester, &KUrlRequester::textChanged, this, &FindThisDeviceConfig::markAsChanged);
}

void FindThisDeviceConfig::defaults()
{
    KCModule::defaults();

    m_ui.soundFileRequester->setText(defaultSound());
    markAsChanged();
}

void FindThisDeviceConfig::load()
{
    KCModule::load();

    const QString ringTone = config()->getString(QStringLiteral("ringtone"), defaultSound());
    m_ui.soundFileRequester->setText(ringTone);
}

void FindThisDeviceConfig::save()
{
    config()->set(QStringLiteral("ringtone"), m_ui.soundFileRequester->text());

    KCModule::save();
}

void FindThisDeviceConfig::playSound(const QUrl &soundUrl)
{
    QMediaPlayer *player = new QMediaPlayer;
    auto audioOutput = new QAudioOutput();
    audioOutput->setVolume(100);
    player->setSource(soundUrl);
    player->setAudioOutput(audioOutput);
    player->play();
    connect(player, &QMediaPlayer::playingChanged, player, &QObject::deleteLater);
}

#include "findthisdevice_config.moc"
#include "moc_findthisdevice_config.cpp"
