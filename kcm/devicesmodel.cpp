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
#include <ksharedconfig.h>

#include <QDebug>
#include <KConfigGroup>

DevicesModel::DevicesModel(QObject *parent)
    : QAbstractListModel(parent)
{

}

DevicesModel::~DevicesModel()
{
}

void DevicesModel::loadPaired()
{

    //TODO: Load from daemon, so we can know if they are currently connected or not

    removeRows(0,rowCount());

    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    const KConfigGroup& known = config->group("devices").group("paired");
    const QStringList& list = known.groupList();

    const QString defaultName("unnamed");

    Q_FOREACH(QString id, list) {

        const KConfigGroup& data = known.group(id);
        const QString& name = data.readEntry<QString>("name",defaultName);

        //qDebug() << id << name;

        addDevice(id,name,Visible);

    }

}

void DevicesModel::addDevice(QString id, QString name, DevicesModel::DeviceStatus status)
{
    int rown = rowCount();
    insertRows(rown,1);
    setData(index(rown,0),QVariant(id),IdModelRole);
    setData(index(rown,0),QVariant(name),NameModelRole);
    setData(index(rown,0),QVariant(PairedConnected),StatusModelRole);
    qDebug() << "Add device" << name;
    emit dataChanged(index(rown,0),index(rown,0));
}


int DevicesModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return 1; //We are not using the second dimension at all
}

QVariant DevicesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_deviceList.count()) {
        return QVariant();
    }
    switch (role) {
            case IconModelRole:
                return QPixmap(); //TODO: Return a pixmap to represent the status
            case IdModelRole:
                return m_deviceList[index.row()].id;
            case NameModelRole:
                return m_deviceList[index.row()].name;
            case StatusModelRole:
                return m_deviceList[index.row()].status;
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
                qDebug() << "Icon can not be assigned to, change status instead";
                break;
            case IdModelRole:
                m_deviceList[index.row()].id = value.toString();
                break;
            case NameModelRole:
                m_deviceList[index.row()].name = value.toString();
                break;
            case StatusModelRole:
                m_deviceList[index.row()].status = (DeviceStatus)value.toInt();
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