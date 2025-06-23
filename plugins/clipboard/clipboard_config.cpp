/**
 * SPDX-FileCopyrightText: 2022 Yuchen Shi <bolshaya_schists@mail.gravitide.co>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "clipboard_config.h"

#include <KPluginFactory>
#include <QSpinBox>
#include <QStringLiteral>

K_PLUGIN_CLASS(ClipboardConfig)

ClipboardConfig::ClipboardConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &args)
    : KdeConnectPluginKcm(parent, data, args)
{
    m_ui.setupUi(widget());

    connect(m_ui.check_autoshare, &QCheckBox::toggled, this, &ClipboardConfig::autoShareChanged);
    connect(m_ui.check_password, &QCheckBox::toggled, this, &ClipboardConfig::markAsChanged);
    connect(m_ui.spinBox_max_clipboard_size, &QSpinBox::valueChanged, this, &ClipboardConfig::maxClipboardSizeChanged);
}

void ClipboardConfig::autoShareChanged()
{
    m_ui.check_password->setEnabled(m_ui.check_autoshare->isChecked());
    markAsChanged();
}

void ClipboardConfig::maxClipboardSizeChanged(int value)
{
    m_ui.spinBox_max_clipboard_size->setValue(value);
    markAsChanged();
}

void ClipboardConfig::defaults()
{
    KCModule::defaults();
    m_ui.check_autoshare->setChecked(true);
    m_ui.check_password->setChecked(true);
    m_ui.spinBox_max_clipboard_size->setValue(50);
    markAsChanged();
}

void ClipboardConfig::load()
{
    KCModule::load();
    // "sendUnknown" is the legacy name for this setting
    bool autoShare = config()->getBool(QStringLiteral("autoShare"), config()->getBool(QStringLiteral("sendUnknown"), true));
    bool password = config()->getBool(QStringLiteral("sendPassword"), true);
    int maxClipboardFileSizeMB = config()->getInt(QStringLiteral("maxClipboardFileSizeMB"), 50);
    m_ui.check_autoshare->setChecked(autoShare);
    m_ui.check_password->setChecked(password);
    m_ui.spinBox_max_clipboard_size->setValue(maxClipboardFileSizeMB);
    autoShareChanged();
}

void ClipboardConfig::save()
{
    config()->set(QStringLiteral("autoShare"), m_ui.check_autoshare->isChecked());
    config()->set(QStringLiteral("sendPassword"), m_ui.check_password->isChecked());
    config()->set(QStringLiteral("maxClipboardFileSizeMB"), m_ui.spinBox_max_clipboard_size->value());
    KCModule::save();
}

#include "clipboard_config.moc"
#include "moc_clipboard_config.cpp"
