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
    Q_EMIT changed(true);
}

void PauseMusicConfig::load()
{
    KCModule::load();
    bool talking = config()->get(QStringLiteral("conditionTalking"), false);
    m_ui->rad_talking->setChecked(talking);
    m_ui->rad_ringing->setChecked(!talking);

    bool pause = config()->get(QStringLiteral("actionPause"), true);
    bool mute = config()->get(QStringLiteral("actionMute"), false);
    m_ui->check_pause->setChecked(pause);
    m_ui->check_mute->setChecked(mute);

    Q_EMIT changed(false);
}

void PauseMusicConfig::save()
{
    config()->set(QStringLiteral("conditionTalking"), m_ui->rad_talking->isChecked());
    config()->set(QStringLiteral("actionPause"), m_ui->check_pause->isChecked());
    config()->set(QStringLiteral("actionMute"), m_ui->check_mute->isChecked());
    KCModule::save();
    Q_EMIT changed(false);
}

#include "pausemusic_config.moc"
