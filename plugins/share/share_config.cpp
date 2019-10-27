/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
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
    m_ui->kurlrequester->setText(config()->getString(QStringLiteral("incoming_path"), standardPath));

    Q_EMIT changed(false);
}

void ShareConfig::save()
{
    config()->set(QStringLiteral("incoming_path"), m_ui->kurlrequester->text());

    KCModule::save();

    Q_EMIT changed(false);
}

#include "share_config.moc"
