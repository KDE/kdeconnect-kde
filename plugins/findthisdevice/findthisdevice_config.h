/**
 * SPDX-FileCopyrightText: 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "ui_findthisdevice_config.h"
#include <kcmplugin/kdeconnectpluginkcm.h>

class FindThisDeviceConfig : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    FindThisDeviceConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &);

    void save() override;
    void load() override;
    void defaults() override;

private:
    void playSound(const QUrl &soundUrl);
    Ui::FindThisDeviceConfigUi m_ui;
};
