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

#include "share_config.h"
#include "ui_share_config.h"

#include <QStandardPaths>

#include <KUrlRequester>
#include <KPluginFactory>

K_PLUGIN_FACTORY(ShareConfigFactory, registerPlugin<ShareConfig>();)

ShareConfig::ShareConfig(QWidget* parent, const QVariantList& args)
    : KdeConnectPluginKcm(parent, args, QStringLiteral("kdeconnect_share_config"))
    , m_ui(new Ui::ShareConfigUi())
{
    m_ui->setupUi(this);
    // xgettext:no-c-format
    m_ui->commentLabel->setTextFormat(Qt::RichText);
    m_ui->commentLabel->setText(i18n("&percnt;1 in the path will be replaced with the specific device name."));

    connect(m_ui->kurlrequester, SIGNAL(textChanged(QString)), this, SLOT(changed()));
}

ShareConfig::~ShareConfig()
{
    delete m_ui;
}

void ShareConfig::defaults()
{
    KCModule::defaults();

    m_ui->kurlrequester->setText(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

    Q_EMIT changed(true);
}

void ShareConfig::load()
{
    KCModule::load();

    const auto standardPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    m_ui->kurlrequester->setText(config()->get<QString>(QStringLiteral("incoming_path"), standardPath));

    Q_EMIT changed(false);
}

void ShareConfig::save()
{
    config()->set(QStringLiteral("incoming_path"), m_ui->kurlrequester->text());

    KCModule::save();

    Q_EMIT changed(false);
}

#include "share_config.moc"
