/**
 * SPDX-FileCopyrightText: 2022 Yuchen Shi <bolshaya_schists@mail.gravitide.co>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboard_config.h"

#include <KPluginFactory>

K_PLUGIN_CLASS(ClipboardConfig)

ClipboardConfig::ClipboardConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KdeConnectPluginKcm(parent, data, args)
{
    m_ui.setupUi(widget());

    connect(m_ui.check_autoshare, &QCheckBox::toggled, this, &ClipboardConfig::autoShareChanged);
    connect(m_ui.check_password, &QCheckBox::toggled, this, &ClipboardConfig::markAsChanged);
}

void ClipboardConfig::autoShareChanged()
{
    m_ui.check_password->setEnabled(m_ui.check_autoshare->isChecked());
    markAsChanged();
}

void ClipboardConfig::defaults()
{
    KCModule::defaults();
    m_ui.check_autoshare->setChecked(true);
    m_ui.check_password->setChecked(true);
    markAsChanged();
}

void ClipboardConfig::load()
{
    KCModule::load();
    // "sendUnknown" is the legacy name for this setting
    bool autoShare = config()->getBool(QStringLiteral("autoShare"), config()->getBool(QStringLiteral("sendUnknown"), true));
    bool password = config()->getBool(QStringLiteral("sendPassword"), true);
    m_ui.check_autoshare->setChecked(autoShare);
    m_ui.check_password->setChecked(password);
    autoShareChanged();
}

void ClipboardConfig::save()
{
    config()->set(QStringLiteral("autoShare"), m_ui.check_autoshare->isChecked());
    config()->set(QStringLiteral("sendPassword"), m_ui.check_password->isChecked());
    KCModule::save();
}

#include "clipboard_config.moc"
#include "moc_clipboard_config.cpp"
