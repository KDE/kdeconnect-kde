/**
 * SPDX-FileCopyrightText: 2022 Yuchen Shi <bolshaya_schists@mail.gravitide.co>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboard_config.h"
#include "ui_clipboard_config.h"

#include <KPluginFactory>

K_PLUGIN_FACTORY(ClipboardConfigFactory, registerPlugin<ClipboardConfig>();)

ClipboardConfig::ClipboardConfig(QWidget* parent, const QVariantList &args)
    : KdeConnectPluginKcm(parent, args, QStringLiteral("kdeconnect_clipboard"))
    , m_ui(new Ui::ClipboardConfigUi())
{
    m_ui->setupUi(this);

    connect(m_ui->check_autoshare, SIGNAL(toggled(bool)), this, SLOT(autoShareChanged()));
    connect(m_ui->check_password, SIGNAL(toggled(bool)), this, SLOT(changed()));
}

ClipboardConfig::~ClipboardConfig()
{
    delete m_ui;
}

void ClipboardConfig::autoShareChanged()
{
    m_ui->check_password->setEnabled(m_ui->check_autoshare->isChecked());
    Q_EMIT changed();
}

void ClipboardConfig::defaults()
{
    KCModule::defaults();
    m_ui->check_autoshare->setChecked(true);
    m_ui->check_password->setChecked(true);
    Q_EMIT changed(true);
}

void ClipboardConfig::load()
{
    KCModule::load();
    // "sendUnknown" is the legacy name for this setting
    bool autoShare = config()->getBool(QStringLiteral("autoShare"), config()->getBool(QStringLiteral("sendUnknown"), true));
    bool password = config()->getBool(QStringLiteral("sendPassword"), true);
    m_ui->check_autoshare->setChecked(autoShare);
    m_ui->check_password->setChecked(password);
    autoShareChanged();
}

void ClipboardConfig::save()
{
    config()->set(QStringLiteral("autoShare"), m_ui->check_autoshare->isChecked());
    config()->set(QStringLiteral("sendPassword"), m_ui->check_password->isChecked());
    KCModule::save();
    Q_EMIT changed(false);
}

#include "clipboard_config.moc"
