/*
 * SPDX-FileCopyrightText: 2016 Aleix Pol Gonzalez <aleixpol@kde.org>
 * SPDX-FileCopyrightText: 2020 Piyush Aggarwal <piyushaggarwal002@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "connectivity_action.h"

ConnectivityAction::ConnectivityAction(DeviceDbusInterface* device)
    : QAction(nullptr)
    , m_connectivityiface(device->id())
{
    setCellularNetworkStrength(m_connectivityiface.cellularNetworkStrength());
    setCellularNetworkType(m_connectivityiface.cellularNetworkType());

    connect(&m_connectivityiface, &ConnectivityReportDbusInterface::refreshedProxy, this, [this]{
        setCellularNetworkStrength(m_connectivityiface.cellularNetworkStrength());
        setCellularNetworkType(m_connectivityiface.cellularNetworkType());
    });

    setIcon(QIcon::fromTheme(QStringLiteral("network-wireless")));

    ConnectivityAction::update();
}

void ConnectivityAction::update()
{
    if (m_cellularNetworkStrength < 0) {
        setText(i18nc("The fallback text to display in case the remote device does not have a cellular connection", "No Cellular Connectivity"));
    } else {
        setText(i18nc("Display the cellular connection type and an approximate percentage of signal strength","%1  | ~%2%", m_cellularNetworkType, m_cellularNetworkStrength * 25));
    }
}

void ConnectivityAction::setCellularNetworkStrength(int cellularNetworkStrength)
{
    m_cellularNetworkStrength = cellularNetworkStrength;
    update();
}

void ConnectivityAction::setCellularNetworkType(QString cellularNetworkType)
{
    m_cellularNetworkType = cellularNetworkType;
    update();
}
