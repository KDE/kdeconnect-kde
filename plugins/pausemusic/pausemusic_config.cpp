/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "pausemusic_config.h"
#include "ui_pausemusic_config.h"

#include <KPluginFactory>

K_PLUGIN_FACTORY(PauseMusicConfigFactory, registerPlugin<PauseMusicConfig>();)

PauseMusicConfig::PauseMusicConfig(QWidget* parent, const QVariantList& args)
    : KdeConnectPluginKcm(parent, args, QStringLiteral("kdeconnect_pausemusic_config"))
    , m_ui(new Ui::PauseMusicConfigUi())
{
    m_ui->setupUi(this);

    connect(m_ui->rad_ringing, SIGNAL(toggled(bool)), this, SLOT(changed()));
    connect(m_ui->rad_talking, SIGNAL(toggled(bool)), this, SLOT(changed()));
    connect(m_ui->check_pause, SIGNAL(toggled(bool)), this, SLOT(changed()));
    connect(m_ui->check_mute, SIGNAL(toggled(bool)), this, SLOT(changed()));
    connect(m_ui->check_resume, SIGNAL(toggled(bool)), this, SLOT(changed()));
}

PauseMusicConfig::~PauseMusicConfig()
{
    delete m_ui;
}

void PauseMusicConfig::defaults()
{
    KCModule::defaults();
    m_ui->rad_talking->setChecked(false);
    m_ui->rad_ringing->setChecked(true);
    m_ui->check_pause->setChecked(true);
    m_ui->check_mute->setChecked(false);
    m_ui->check_resume->setChecked(true);
    Q_EMIT changed(true);
}

void PauseMusicConfig::load()
{
    KCModule::load();
    bool talking = config()->getBool(QStringLiteral("conditionTalking"), false);
    m_ui->rad_talking->setChecked(talking);
    m_ui->rad_ringing->setChecked(!talking);

    bool pause = config()->getBool(QStringLiteral("actionPause"), true);
    bool mute = config()->getBool(QStringLiteral("actionMute"), false);
    m_ui->check_pause->setChecked(pause);
    m_ui->check_mute->setChecked(mute);

    const bool autoResume = config()->getBool(QStringLiteral("actionResume"), true);
    m_ui->check_resume->setChecked(autoResume);

    Q_EMIT changed(false);
}

void PauseMusicConfig::save()
{
    config()->set(QStringLiteral("conditionTalking"), m_ui->rad_talking->isChecked());
    config()->set(QStringLiteral("actionPause"), m_ui->check_pause->isChecked());
    config()->set(QStringLiteral("actionMute"), m_ui->check_mute->isChecked());
    config()->set(QStringLiteral("actionResume"), m_ui->check_resume->isChecked());
    KCModule::save();
    Q_EMIT changed(false);
}

#include "pausemusic_config.moc"
