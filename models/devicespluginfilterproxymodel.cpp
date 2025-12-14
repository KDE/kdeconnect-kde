/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "devicespluginfilterproxymodel.h"

bool DevicesPluginFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
    const QModelIndex idx = sourceModel()->index(source_row, 0, source_parent);
    auto device = qobject_cast<DeviceDbusInterface *>(idx.data(DevicesModel::DeviceRole).value<QObject *>());
    return device->hasPlugin(m_pluginFilter);
}

void DevicesPluginFilterProxyModel::setPluginFilter(const QString plugin)
{
    m_pluginFilter = plugin;
}

QString DevicesPluginFilterProxyModel::pluginFilter() const
{
    return m_pluginFilter;
}

int DevicesPluginFilterProxyModel::rowForDevice(const QString &id) const
{
    for (int i = 0, c = rowCount(); i < c; ++i) {
        if (data(index(i, 0), DevicesModel::IdModelRole).value<QString>() == id) {
            return i;
        }
    }
    return -1;
}

#include "moc_devicespluginfilterproxymodel.cpp"
