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

#include "devicesmodel.h"
#include <ksharedconfig.h>

#include <QDebug>
#include <QDBusInterface>

#include <KConfigGroup>
#include <KIcon>

bool fetchNotifications = true;

DevicesModel::DevicesModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(new DaemonDbusInterface(this))
{

    connect(m_dbusInterface, SIGNAL(deviceAdded(QString)),
            this, SLOT(deviceAdded(QString)));
    connect(m_dbusInterface, SIGNAL(deviceVisibilityChanged(QString,bool)),
            this, SLOT(deviceStatusChanged(QString)));
    connect(m_dbusInterface, SIGNAL(deviceRemoved(QString)),
            this, SLOT(deviceRemoved(QString)));

    refreshDeviceList();

}

DevicesModel::~DevicesModel()
{
}

void DevicesModel::deviceAdded(const QString& id)
{
    //TODO: Actually add instead of refresh
    Q_UNUSED(id);
    refreshDeviceList();
}

void DevicesModel::deviceRemoved(const QString& id)
{
    //TODO: Actually remove instead of refresh
    Q_UNUSED(id);
    refreshDeviceList();
}

void DevicesModel::deviceStatusChanged(const QString& id)
{
    Q_UNUSED(id);
    //FIXME: Emitting dataChanged does not invalidate the view, refreshDeviceList does.
    //Q_EMIT dataChanged(index(0),index(rowCount()));
    refreshDeviceList();
}

void DevicesModel::refreshDeviceList()
{

    if (m_deviceList.count() > 0) {
        beginRemoveRows(QModelIndex(), 0, m_deviceList.size() - 1);
        m_deviceList.clear();
        endRemoveRows();
    }

    QList<QString> deviceIds = m_dbusInterface->devices();
    beginInsertRows(QModelIndex(), 0, deviceIds.size()-1);
    Q_FOREACH(const QString& id, deviceIds) {
        DeviceDbusInterface* deviceDbusInterface = new DeviceDbusInterface(id,this);
        m_deviceList.append(deviceDbusInterface);
    }
    endInsertRows();


    Q_EMIT dataChanged(index(0), index(deviceIds.count()));

}

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

    if (!index.isValid()
        || index.row() < 0
        || index.row() >= m_deviceList.count()
        || !m_deviceList[index.row()]->isValid())
    {
        return QVariant();
    }

    DeviceDbusInterface* device = m_deviceList[index.row()];

    //FIXME: This function gets called lots of times, producing lots of dbus calls. Add a cache.
    switch (role) {
        case IconModelRole: {
            bool paired = device->paired();
            bool reachable = device->reachable();
            QString icon = reachable? (paired? "user-online" : "user-busy") : "user-offline";
            return KIcon(icon).pixmap(32, 32);
        }
        case IdModelRole:
            return QString(device->id());
        case NameModelRole:
            return QString(device->name());
        case Qt::ToolTipRole:
            return QVariant(); //To implement
        case StatusModelRole: {
            int status = StatusUnknown;
            if (device->reachable()) {
                status |= StatusReachable;
                if (device->paired()) status |= StatusPaired;
            }
            return status;
        }
        default:
             return QVariant();
    }
}

DeviceDbusInterface* DevicesModel::getDevice(const QModelIndex& index)
{
    if (!index.isValid()) {
        return NULL;
    }

    int row = index.row();
    if (row < 0 || row >= m_deviceList.size()) {
        return NULL;
    }

    return m_deviceList[row];
}

int DevicesModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_deviceList.count();
}

