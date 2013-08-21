/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#include "devicessortproxymodel.h"

#include "libkdeconnect/dbusinterfaces.h"
#include "libkdeconnect/devicesmodel.h"

#include <QDebug>

DevicesSortProxyModel::DevicesSortProxyModel(DevicesModel* devicesModel)
    : QSortFilterProxyModel(devicesModel)
{
    setSourceModel(devicesModel);
    setSortRole(DevicesModel::StatusModelRole);
    connect(devicesModel, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SLOT(sourceDataChanged(QModelIndex,QModelIndex)));
    sort(0);
}

void DevicesSortProxyModel::sourceDataChanged(QModelIndex , QModelIndex )
{
    sort(0);
}

bool DevicesSortProxyModel::lessThan(const QModelIndex& left, const QModelIndex& right) const
{
    QVariant leftData = sourceModel()->data(left, Qt::InitialSortOrderRole);
    QVariant rightData = sourceModel()->data(right, Qt::InitialSortOrderRole);

    return leftData.toInt() > rightData.toInt();
}

bool DevicesSortProxyModel::filterAcceptsRow(int source_row, const QModelIndex& source_parent) const
{
    Q_UNUSED(source_row);
    Q_UNUSED(source_parent);
    //Possible to-do: Implement filter
    return true;
}
