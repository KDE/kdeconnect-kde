/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pausemusic_config.h"

#include <KPluginFactory>

K_PLUGIN_CLASS(PauseMusicConfig)

PauseMusicConfig::PauseMusicConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KdeConnectPluginKcm(parent, data, args)
{
    m_ui.setupUi(widget());

    connect(m_ui.rad_ringing, &QCheckBox::toggled, this, &PauseMusicConfig::markAsChanged);
    connect(m_ui.rad_talking, &QCheckBox::toggled, this, &PauseMusicConfig::markAsChanged);
    connect(m_ui.check_pause, &QCheckBox::toggled, this, &PauseMusicConfig::markAsChanged);
    connect(m_ui.check_mute, &QCheckBox::toggled, this, &PauseMusicConfig::markAsChanged);
    connect(m_ui.check_resume, &QCheckBox::toggled, this, &PauseMusicConfig::markAsChanged);
}

void PauseMusicConfig::defaults()
{
    KCModule::defaults();
    m_ui.rad_talking->setChecked(false);
    m_ui.rad_ringing->setChecked(true);
    m_ui.check_pause->setChecked(true);
    m_ui.check_mute->setChecked(false);
    m_ui.check_resume->setChecked(true);
    markAsChanged();
}

void PauseMusicConfig::load()
{
    KCModule::load();
    bool talking = config()->getBool(QStringLiteral("conditionTalking"), false);
    m_ui.rad_talking->setChecked(talking);
    m_ui.rad_ringing->setChecked(!talking);

    bool pause = config()->getBool(QStringLiteral("actionPause"), true);
    bool mute = config()->getBool(QStringLiteral("actionMute"), false);
    m_ui.check_pause->setChecked(pause);
    m_ui.check_mute->setChecked(mute);

    const bool autoResume = config()->getBool(QStringLiteral("actionResume"), true);
    m_ui.check_resume->setChecked(autoResume);
}

void PauseMusicConfig::save()
{
    config()->set(QStringLiteral("conditionTalking"), m_ui.rad_talking->isChecked());
    config()->set(QStringLiteral("actionPause"), m_ui.check_pause->isChecked());
    config()->set(QStringLiteral("actionMute"), m_ui.check_mute->isChecked());
    config()->set(QStringLiteral("actionResume"), m_ui.check_resume->isChecked());
    KCModule::save();
}

#include "moc_pausemusic_config.cpp"
#include "pausemusic_config.moc"
