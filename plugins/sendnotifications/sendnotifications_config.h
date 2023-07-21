/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include "kcmplugin/kdeconnectpluginkcm.h"

namespace Ui
{
class SendNotificationsConfigUi;
}

class NotifyingApplicationModel;

class SendNotificationsConfig : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    SendNotificationsConfig(QObject *parent, const KPluginMetaData &data, const QVariantList &);
    ~SendNotificationsConfig() override;

    void save() override;
    void load() override;
    void defaults() override;

private:
    void loadApplications();
    Ui::SendNotificationsConfigUi *m_ui;
    NotifyingApplicationModel *appModel;
};
