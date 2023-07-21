/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PAUSEMUSIC_CONFIG_H
#define PAUSEMUSIC_CONFIG_H

#include "kcmplugin/kdeconnectpluginkcm.h"

namespace Ui
{
class PauseMusicConfigUi;
}

class PauseMusicConfig : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    PauseMusicConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &);
    ~PauseMusicConfig() override;

    void save() override;
    void load() override;
    void defaults() override;

private:
    Ui::PauseMusicConfigUi *m_ui;
};

#endif
