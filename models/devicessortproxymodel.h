/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DEVICESSORTPROXYMODEL_H
#define DEVICESSORTPROXYMODEL_H

#include "kdeconnectmodels_export.h"
#include <QSortFilterProxyModel>

class DevicesModel;

class KDECONNECTMODELS_EXPORT DevicesSortProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit DevicesSortProxyModel(DevicesModel *devicesModel = nullptr);
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

#endif // DEVICESSORTPROXYMODEL_H
