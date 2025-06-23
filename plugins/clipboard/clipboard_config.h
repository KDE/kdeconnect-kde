/**
 * SPDX-FileCopyrightText: 2022 Yuchen Shi <bolshaya_schists@mail.gravitide.co>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "kcmplugin/kdeconnectpluginkcm.h"
#include "ui_clipboard_config.h"

class ClipboardConfig : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    ClipboardConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &);

    void save() override;
    void load() override;
    void defaults() override;

private:
    void autoShareChanged();
    void maxClipboardSizeChanged(int value);

    Ui::ClipboardConfigUi m_ui;
};
