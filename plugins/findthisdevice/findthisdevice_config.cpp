/**
 * SPDX-FileCopyrightText: 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "findthisdevice_config.h"
#include "findthisdeviceplugin.h"

#include "ui_findthisdevice_config.h"
// KF
#include <KLocalizedString>
#include <KPluginFactory>
// Qt
#include <QMediaPlayer>
#include <QStandardPaths>

K_PLUGIN_FACTORY(FindThisDeviceConfigFactory, registerPlugin<FindThisDeviceConfig>();)

FindThisDeviceConfig::FindThisDeviceConfig(QWidget *parent, const QVariantList &args)
    : KdeConnectPluginKcm(parent, args, QStringLiteral("kdeconnect_findthisdevice"))
    , m_ui(new Ui::FindThisDeviceConfigUi())
{
    m_ui->setupUi(this);

    const QStringList soundDirs = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation, QStringLiteral("sounds"), QStandardPaths::LocateDirectory);
    if (!soundDirs.isEmpty()) {
        m_ui->soundFileRequester->setStartDir(QUrl::fromLocalFile(soundDirs.last()));
    }

    connect(m_ui->playSoundButton, &QToolButton::clicked, this, &FindThisDeviceConfig::playSound);
    connect(m_ui->soundFileRequester, &KUrlRequester::textChanged, this, &FindThisDeviceConfig::markAsChanged);
}

FindThisDeviceConfig::~FindThisDeviceConfig()
{
    delete m_ui;
}

void FindThisDeviceConfig::defaults()
{
    KCModule::defaults();

    m_ui->soundFileRequester->setText(defaultSound());

    Q_EMIT changed(true);
}

void FindThisDeviceConfig::load()
{
    KCModule::load();

    const QString ringTone = config()->getString(QStringLiteral("ringtone"), defaultSound());
    m_ui->soundFileRequester->setText(ringTone);

    Q_EMIT changed(false);
}

void FindThisDeviceConfig::save()
{
    config()->set(QStringLiteral("ringtone"), m_ui->soundFileRequester->text());

    KCModule::save();

    Q_EMIT changed(false);
}

void FindThisDeviceConfig::playSound()
{
    const QUrl soundURL = m_ui->soundFileRequester->url();

    if (soundURL.isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_FINDTHISDEVICE) << "Not playing sound, no valid ring tone specified.";
    } else {
        QMediaPlayer *player = new QMediaPlayer;
        player->setAudioRole(QAudio::Role(QAudio::NotificationRole));
        player->setMedia(soundURL);
        player->setVolume(100);
        player->play();
        connect(player, &QMediaPlayer::stateChanged, player, &QObject::deleteLater);
    }
}

#include "findthisdevice_config.moc"
