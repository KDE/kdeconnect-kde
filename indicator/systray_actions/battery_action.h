/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2020 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef BATTERYACTION_H
#define BATTERYACTION_H

#include <KLocalizedString>
#include <QFileDialog>
#include <QMenu>

#include "dbusinterfaces/dbusinterfaces.h"

#include <dbushelper.h>

class BatteryAction : public QAction
{
    Q_OBJECT
public:
    BatteryAction(DeviceDbusInterface *device);
    void update();
private Q_SLOTS:
    void setCharge(int charge);
    void setCharging(bool charging);

private:
    BatteryDbusInterface m_batteryIface;
    int m_charge = -1;
    bool m_charging = false;
};

#endif // BATTERYACTION_H
