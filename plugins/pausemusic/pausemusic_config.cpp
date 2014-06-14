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

#include <KPluginFactory>
#include <KSharedConfig>
#include <KConfigGroup>

#include <core/kdebugnamespace.h>

#include "ui_pausemusic_config.h"

K_PLUGIN_FACTORY(PauseMusicConfigFactory, registerPlugin<PauseMusicConfig>();)
K_EXPORT_PLUGIN(PauseMusicConfigFactory("kdeconnect_pausemusic_config", "kdeconnect-kded"))

PauseMusicConfig::PauseMusicConfig(QWidget *parent, const QVariantList& )
    : KCModule(PauseMusicConfigFactory::componentData(), parent)
    , m_ui(new Ui::PauseMusicConfigUi())
    , m_cfg(KSharedConfig::openConfig("kdeconnect/plugins/pausemusic"))
{
    m_ui->setupUi(this);

    connect(m_ui->rad_ringing, SIGNAL(toggled(bool)), this, SLOT(changed()));
    connect(m_ui->rad_talking, SIGNAL(toggled(bool)), this, SLOT(changed()));
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
    m_ui->check_mute->setChecked(false);
    Q_EMIT changed(true);
}


void PauseMusicConfig::load()
{
    KCModule::load();
    bool talking = m_cfg->group("pause_condition").readEntry("talking_only", false);
    m_ui->rad_talking->setChecked(talking);
    m_ui->rad_ringing->setChecked(!talking);
    bool use_mute = m_cfg->group("use_mute").readEntry("use_mute", false);
    m_ui->check_mute->setChecked(use_mute);
    Q_EMIT changed(false);
}


void PauseMusicConfig::save()
{
    m_cfg->group("pause_condition").writeEntry("talking_only", m_ui->rad_talking->isChecked());
    m_cfg->group("use_mute").writeEntry("use_mute", m_ui->check_mute->isChecked());
    KCModule::save();

    Q_EMIT changed(false);
}

