/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SHARE_CONFIG_H
#define SHARE_CONFIG_H

#include "kcmplugin/kdeconnectpluginkcm.h"

namespace Ui
{
class ShareConfigUi;
}

class ShareConfig : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    ShareConfig(QObject *parent, const QVariantList &);
    ~ShareConfig() override;

public Q_SLOTS:
    void save() override;
    void load() override;
    void defaults() override;

private:
    Ui::ShareConfigUi *m_ui;
};

#endif
