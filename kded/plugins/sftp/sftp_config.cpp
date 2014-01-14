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

#include "sftp_config.h"

#include <KConfigGroup>
#include <KGlobalSettings>
#include <KPluginFactory>
#include <KSharedConfig>
#include <KStandardDirs>

#include "../../kdebugnamespace.h"

#include "ui_sftp_config.h"

K_PLUGIN_FACTORY(SftpConfigFactory, registerPlugin<SftpConfig>();)
K_EXPORT_PLUGIN(SftpConfigFactory("kdeconnect_sftp_config", "kdeconnect_sftp_config"))

SftpConfig::SftpConfig(QWidget *parent, const QVariantList& )
    : KCModule(SftpConfigFactory::componentData(), parent)
    , ui(new Ui::SftpConfigUi())
    , cfg(KSharedConfig::openConfig("kdeconnect/plugins/sftp"))
{
    ui->setupUi(this);

    ui->check->setIcon(KIconLoader::global()->loadIcon("view-refresh", KIconLoader::Dialog));
    connect(ui->check, SIGNAL(clicked(bool)), this, SLOT(refresh()));
    refresh();
}

SftpConfig::~SftpConfig()
{
    delete ui;
}

void SftpConfig::refresh()
{
    const QString sshfs = KStandardDirs::findExe("sshfs");
    
    if (sshfs.isEmpty())
    {
        ui->label->setText(i18n("sshfs not found in PATH"));      
        ui->pixmap->setPixmap(KIconLoader::global()->loadIcon("dialog-error", KIconLoader::Dialog));
    }
    else
    {
        ui->label->setText(i18n("sshfs found at %1").arg(sshfs));        
        ui->pixmap->setPixmap(KIconLoader::global()->loadIcon("dialog-ok", KIconLoader::Dialog));  
    }
}

void SftpConfig::defaults()
{
    KCModule::defaults();
    refresh();
    //Q_EMIT changed(true);
}


void SftpConfig::load()
{
    KCModule::load();
    refresh()    ;
    //Q_EMIT changed(false);
}


void SftpConfig::save()
{
    refresh();
    KCModule::save();
    //Q_EMIT changed(false);
}

