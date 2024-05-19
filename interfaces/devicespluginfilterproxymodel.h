/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DEVICESPLUGINFILTERPROXYMODEL_H
#define DEVICESPLUGINFILTERPROXYMODEL_H

#include "dbusinterfaces.h"
#include "devicesmodel.h"
#include "devicessortproxymodel.h"

class KDECONNECTINTERFACES_EXPORT DevicesPluginFilterProxyModel : public DevicesSortProxyModel
{
    Q_OBJECT
    Q_PROPERTY(QString pluginFilter READ pluginFilter WRITE setPluginFilter)
public:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
    void setPluginFilter(const QString plugin);
    QString pluginFilter() const;
    int rowForDevice(const QString &id) const;

private:
    QString m_pluginFilter;
};

#endif
