/**
 * Copyright 2014 Samoilenko Yuri<kinnalru@gmail.com>
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

#include "sftp_config.h"

#include <KConfigGroup>
#include <KGlobalSettings>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KStandardDirs>

#include "sftpplugin.h"
#include <core/kdebugnamespace.h>

#include "ui_sftp_config.h"

K_PLUGIN_FACTORY(SftpConfigFactory, registerPlugin<SftpConfig>();)
K_EXPORT_PLUGIN(SftpConfigFactory("kdeconnect_sftp_config", "kdeconnect-kded"))

SftpConfig::SftpConfig(QWidget *parent, const QVariantList& )
    : KCModule(SftpConfigFactory::componentData(), parent)
    , m_ui(new Ui::SftpConfigUi())
    , m_cfg(SftpConfig::config())
{
    m_ui->setupUi(this);

    m_ui->refresh->setIcon(KIconLoader::global()->loadIcon("view-refresh", KIconLoader::Dialog));
    m_ui->pixmap->setPixmap(KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Dialog));
    
    connect(m_ui->refresh, SIGNAL(clicked(bool)), this, SLOT(checkSshfs()));
    
    connect(m_ui->idle, SIGNAL(toggled(bool)), this, SLOT(changed()));
    connect(m_ui->timeout, SIGNAL(valueChanged(int)), this, SLOT(changed()));
}

SftpConfig::~SftpConfig()
{
}

void SftpConfig::checkSshfs()
{
    m_ui->error->setVisible(KStandardDirs::findExe("sshfs").isEmpty());
}

void SftpConfig::defaults()
{
    KCModule::defaults();
    
    checkSshfs();
    m_ui->idle->setChecked(m_cfg->group("main").readEntry("idle", true));
    m_ui->timeout->setValue(m_cfg->group("main").readEntry("idletimeout", 10));
    
    Q_EMIT changed(true);
}

void SftpConfig::load()
{
    KCModule::load();
    
    checkSshfs();
    m_ui->idle->setChecked(m_cfg->group("main").readEntry("idle", true));
    m_ui->timeout->setValue(m_cfg->group("main").readEntry("idletimeout", 10));
    
    Q_EMIT changed(false);
}


void SftpConfig::save()
{
    checkSshfs();
    m_cfg->group("main").writeEntry("idle", m_ui->idle->isChecked());
    m_cfg->group("main").writeEntry("idletimeout", m_ui->timeout->value());

    KCModule::save();
    Q_EMIT changed(false);
}


