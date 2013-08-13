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
#include <qdbusinterface.h>
#include <KConfigGroup>
#include <kicon.h>

DevicesModel::DevicesModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(new DaemonDbusInterface(this))
{
    QList<QString> deviceIds = m_dbusInterface->devices();
    connect(m_dbusInterface,SIGNAL(newDeviceAdded(QString)),this,SLOT(deviceAdded(QString)));
    Q_FOREACH(const QString& id, deviceIds) {
        deviceAdded(id);
    }
    connect(m_dbusInterface,SIGNAL(deviceStatusChanged(QString)),this,SLOT(deviceStatusChanged(QString)));
    //connect(m_dbusInterface,SIGNAL(deviceRemoved(QString)),this,SLOT(deviceRemoved(QString));
}

DevicesModel::~DevicesModel()
{
}

void DevicesModel::deviceStatusChanged(const QString& id)
{
    Q_UNUSED(id);
    qDebug() << "deviceStatusChanged";
    emit dataChanged(index(0),index(rowCount()));
}

void DevicesModel::deviceAdded(const QString& id)
{
    /*
    beginInsertRows(QModelIndex(), rowCount(), rowCount() + 1);
    m_deviceList.append(new DeviceDbusInterface(id,this));
    endInsertRows();
    */

    //Full refresh
    Q_UNUSED(id);
    if (m_deviceList.count() > 0) {
        beginRemoveRows(QModelIndex(), 0, m_deviceList.count() - 1);
        m_deviceList.clear();
        endRemoveRows();
    }
    QList<QString> deviceIds = m_dbusInterface->devices();
    Q_FOREACH(const QString& id, deviceIds) {
        beginInsertRows(QModelIndex(), rowCount(), rowCount() + 1);
        m_deviceList.append(new DeviceDbusInterface(id,this));
        endInsertRows();
    }

}

/*
bool DevicesModel::insertRows(int row, int count, const QModelIndex &parent)
{
    if (row < 0 || row > m_deviceList.count() || count < 1) {
        return false;
    }
    beginInsertRows(parent, row, row + count - 1);
    for (int i = row; i < row + count; ++i) {
        m_deviceList.insert(i, new Device());
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
*/
QVariant DevicesModel::data(const QModelIndex &index, int role) const
{

    if (!m_dbusInterface->isValid()) {
        switch (role) {
            case IconModelRole:
                return KIcon("dialog-close").pixmap(32, 32);
            case NameModelRole:
                return QString("KDED not running");
            default:
                return QVariant();
        }
    }


    if (!index.isValid() || index.row() < 0 || index.row() >= m_deviceList.count()) {
        return QVariant();
    }


    //FIXME: This function gets called lots of times per second, producing lots of dbus calls
    switch (role) {
        case IconModelRole: {
            bool paired = m_deviceList[index.row()]->paired();
            bool reachable = m_deviceList[index.row()]->reachable();
            QString icon = reachable? (paired? "user-online" : "user-busy") : "user-offline";
            return KIcon(icon).pixmap(32, 32);
        }
        case IdModelRole:
            return QString(m_deviceList[index.row()]->id());
        case NameModelRole:
            return QString(m_deviceList[index.row()]->name());
        default:
             return QVariant();
    }
}

DeviceDbusInterface* DevicesModel::getDevice(const QModelIndex& index)
{
    return m_deviceList[index.row()];
}

int DevicesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_deviceList.count();
}

