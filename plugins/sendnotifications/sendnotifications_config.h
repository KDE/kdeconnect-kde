/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SENDNOTIFICATIONS_CONFIG_H
#define SENDNOTIFICATIONS_CONFIG_H

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
    SendNotificationsConfig(QObject *parent, const QVariantList &);
    ~SendNotificationsConfig() override;

public Q_SLOTS:
    void save() override;
    void load() override;
    void defaults() override;

private Q_SLOTS:
    void loadApplications();

private:
    Ui::SendNotificationsConfigUi *m_ui;
    NotifyingApplicationModel *appModel;
};

#endif
