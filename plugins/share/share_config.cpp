/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "share_config.h"

#include <QStandardPaths>

#include <KPluginFactory>
#include <KUrlRequester>

K_PLUGIN_CLASS(ShareConfig)

ShareConfig::ShareConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KdeConnectPluginKcm(parent, data, args)
{
    m_ui.setupUi(widget());
    // xgettext:no-c-format
    m_ui.commentLabel->setTextFormat(Qt::RichText);
    m_ui.commentLabel->setText(i18n("&percnt;1 in the path will be replaced with the specific device name."));

    connect(m_ui.kurlrequester, &KUrlRequester::textChanged, this, &ShareConfig::markAsChanged);
}

void ShareConfig::defaults()
{
    KCModule::defaults();

    m_ui.kurlrequester->setText(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));

    markAsChanged();
}

void ShareConfig::load()
{
    KCModule::load();

    const auto standardPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    m_ui.kurlrequester->setText(config()->getString(QStringLiteral("incoming_path"), standardPath));
}

void ShareConfig::save()
{
    KCModule::save();
    config()->set(QStringLiteral("incoming_path"), m_ui.kurlrequester->text());
}

#include "moc_share_config.cpp"
#include "share_config.moc"
