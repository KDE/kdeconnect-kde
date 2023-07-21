/**
 * SPDX-FileCopyrightText: 2018 Friedrich W. H. Kossebau <kossebau@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef FINDTHISDEVICE_CONFIG_H
#define FINDTHISDEVICE_CONFIG_H

#include <kcmplugin/kdeconnectpluginkcm.h>

namespace Ui
{
class FindThisDeviceConfigUi;
}

class FindThisDeviceConfig : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    FindThisDeviceConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &);
    ~FindThisDeviceConfig() override;

    void save() override;
    void load() override;
    void defaults() override;

private:
    void playSound(const QUrl &soundUrl);
    Ui::FindThisDeviceConfigUi *m_ui;
};

#endif
