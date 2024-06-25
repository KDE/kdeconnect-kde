/**
 * SPDX-FileCopyrightText: 2024 David Redondo <kde@david-redondo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "kcmplugin/kdeconnectpluginkcm.h"
#include "ui_shareinputdevices_config.h"

class DaemonDbusInterface;

class ShareInputDevicesConfig : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    ShareInputDevicesConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &args);

    void save() override;
    void load() override;
    void defaults() override;

private:
    void updateState();
    void checkEdge();
    Ui::ShareInputDevicesConfig m_ui;
    DaemonDbusInterface *m_daemon;
};
