/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2020 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CONNECTIVITYACTION_H
#define CONNECTIVITYACTION_H

#include <KLocalizedString>
#include <QFileDialog>
#include <QMenu>

#include "dbusinterfaces/dbusinterfaces.h"

#include <dbushelper.h>

namespace connectivity_action
{
const QStringList networkTypesWithIcons{
    // contains the name of network types that have an associated icon in Breeze-icons
    QStringLiteral("EDGE"),
    QStringLiteral("GPRS"),
    QStringLiteral("HSPA"),
    QStringLiteral("LTE"),
    QStringLiteral("UMTS"),
};
}

class ConnectivityAction : public QAction
{
    Q_OBJECT
public:
    ConnectivityAction(DeviceDbusInterface *device);
    void update();
private Q_SLOTS:
    void setCellularNetworkStrength(int cellularNetworkStrength);
    void setCellularNetworkType(QString cellularNetworkType);

private:
    ConnectivityReportDbusInterface m_connectivityiface;
    QString m_cellularNetworkType;
    int m_cellularNetworkStrength = -1;
};

#endif // CONNECTIVITYACTION_H
