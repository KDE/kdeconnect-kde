/**
 * Copyright 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef NOTIFICATIONS_CONFIG_H
#define NOTIFICATIONS_CONFIG_H

#include "kcmplugin/kdeconnectpluginkcm.h"

namespace Ui {
    class NotificationsConfigUi;
}

class NotifyingApplicationModel;

class NotificationsConfig
    : public KdeConnectPluginKcm
{
    Q_OBJECT
public:
    NotificationsConfig(QWidget *parent, const QVariantList&);
    virtual ~NotificationsConfig();

public Q_SLOTS:
    virtual void save() override;
    virtual void load() override;
    virtual void defaults() override;

private Q_SLOTS:
    void loadApplications();

private:
    Ui::NotificationsConfigUi* m_ui;
    NotifyingApplicationModel* appModel;

};

#endif
