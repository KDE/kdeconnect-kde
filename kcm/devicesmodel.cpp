/*
 * <one line to give the library's name and an idea of what it does.>
 * Copyright 2013  <copyright holder> <email>
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
 *
 */

#include "devicesmodel.h"


DevicesModel::DevicesModel(QObject *parent)
    : QAbstractItemModel(parent)
{
}

DevicesModel::~DevicesModel()
{
}

int DevicesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 1;
}

QVariant DevicesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_deviceList.count()) {
        return QVariant();
    }
    switch (role) {
            case IconModelRole:
                //return m_deviceList[index.row()].m_icon;
            case NameModelRole:
                //return m_deviceList[index.row()].m_device->name();
            case AliasModelRole:
                //return m_deviceList[index.row()].m_device->alias();
            case DeviceTypeModelRole:
                //return m_deviceList[index.row()].m_deviceType;
            case DeviceModelRole:
                //return QVariant::fromValue<void*>(m_deviceList[index.row()].m_device);
            default:
                break;
    }
    return QVariant();
}

bool DevicesModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_deviceList.count()) {
        return false;
    }
    switch (role) {
            case IconModelRole:
                //m_deviceList[index.row()].m_icon = value.value<QPixmap>();
                break;
            case DeviceTypeModelRole:
                //m_deviceList[index.row()].m_deviceType = value.toString();
                break;
            case DeviceModelRole: {
                    //Device *const device = static_cast<Device*>(value.value<void*>());
                    //m_deviceList[index.row()].m_device = device;
                    //connect(device, SIGNAL(propertyChanged(QString,QVariant)),this, SIGNAL(layoutChanged()));
                }
                break;
            default:
                return false;
    }
    emit dataChanged(index, index);
    return true;
}

QModelIndex DevicesModel::index(int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    if (row < 0 || row >= m_deviceList.count() || column != 0) {
        return QModelIndex();
    }

    return createIndex(row, column);
}

QModelIndex DevicesModel::parent(const QModelIndex &index) const
{
    Q_UNUSED(index);

    return QModelIndex();
}

int DevicesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_deviceList.count();
}

bool DevicesModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || row > m_deviceList.count() || count < 1) {
        return false;
    }
    beginInsertRows(parent, row, row + count - 1);
    for (int i = row; i < row + count; ++i) {
        m_deviceList.insert(i, Device());
    }
    endInsertRows();
    return true;
}

bool DevicesModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || row > m_deviceList.count() || count < 1) {
        return false;
    }
    beginRemoveRows(parent, row, row + count - 1);
    for (int i = row; i < row + count; ++i) {
        m_deviceList.removeAt(row);
    }
    endRemoveRows();
    return true;
}