/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "devicessortproxymodel.h"

#include "dbusinterfaces.h"
#include "devicesmodel.h"

DevicesSortProxyModel::DevicesSortProxyModel(DevicesModel *devicesModel)
    : QSortFilterProxyModel(devicesModel)
{
    setSourceModel(devicesModel);
    setSortRole(DevicesModel::StatusModelRole);
    sort(0);
}

bool DevicesSortProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    QAbstractItemModel *model = sourceModel();
    Q_ASSERT(qobject_cast<DevicesModel *>(model));

    // Show connected devices first
    int statusLeft = model->data(left, DevicesModel::StatusModelRole).toInt();
    int statusRight = model->data(right, DevicesModel::StatusModelRole).toInt();

    if (statusLeft != statusRight) {
        return statusLeft > statusRight;
    }

    // Fallback to alphabetical order
    QString nameLeft = model->data(left, DevicesModel::NameModelRole).toString();
    QString nameRight = model->data(right, DevicesModel::NameModelRole).toString();

    return nameLeft < nameRight;
}

bool DevicesSortProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    Q_UNUSED(source_row);
    Q_UNUSED(source_parent);
    // Possible to-do: Implement filter
    return true;
}

#include "moc_devicessortproxymodel.cpp"
