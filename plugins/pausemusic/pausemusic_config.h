/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "kcmplugin/kdeconnectpluginkcm.h"
#include "ui_pausemusic_config.h"

class PauseMusicConfig : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    PauseMusicConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &);

    void save() override;
    void load() override;
    void defaults() override;

private:
    Ui::PauseMusicConfigUi m_ui;
};
