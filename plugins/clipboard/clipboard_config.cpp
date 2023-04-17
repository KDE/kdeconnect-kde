/**
 * SPDX-FileCopyrightText: 2022 Yuchen Shi <bolshaya_schists@mail.gravitide.co>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboard_config.h"
#include "ui_clipboard_config.h"

#include <KPluginFactory>

K_PLUGIN_FACTORY(ClipboardConfigFactory, registerPlugin<ClipboardConfig>();)

ClipboardConfig::ClipboardConfig(QObject *parent, const QVariantList &args)
    : KdeConnectPluginKcm(parent, args, QStringLiteral("kdeconnect_clipboard"))
    , m_ui(new Ui::ClipboardConfigUi())
{
    m_ui->setupUi(widget());

    connect(m_ui->check_unknown, SIGNAL(toggled(bool)), this, SLOT(changed()));
    connect(m_ui->check_password, SIGNAL(toggled(bool)), this, SLOT(changed()));
}

ClipboardConfig::~ClipboardConfig()
{
    delete m_ui;
}

void ClipboardConfig::defaults()
{
    KCModule::defaults();
    m_ui->check_unknown->setChecked(true);
    m_ui->check_password->setChecked(true);
    markAsChanged();
}

void ClipboardConfig::load()
{
    KCModule::load();
    bool unknown = config()->getBool(QStringLiteral("sendUnknown"), true);
    bool password = config()->getBool(QStringLiteral("sendPassword"), true);
    m_ui->check_unknown->setChecked(unknown);
    m_ui->check_password->setChecked(password);
}

void ClipboardConfig::save()
{
    config()->set(QStringLiteral("sendUnknown"), m_ui->check_unknown->isChecked());
    config()->set(QStringLiteral("sendPassword"), m_ui->check_password->isChecked());
    KCModule::save();
}

#include "clipboard_config.moc"
