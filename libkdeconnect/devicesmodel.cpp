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
#include "modeltest.h"
#include <ksharedconfig.h>

#include <QDebug>
#include <QDBusInterface>

#include <KConfigGroup>
#include <KIcon>

DevicesModel::DevicesModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(new DaemonDbusInterface(this))
    , m_displayFilter(DevicesModel::StatusUnknown)
{

    //new ModelTest(this, this);

    connect(this, SIGNAL(rowsRemoved(QModelIndex, int, int)),
            this, SIGNAL(rowsChanged()));
    connect(this, SIGNAL(rowsInserted(QModelIndex, int, int)),
            this, SIGNAL(rowsChanged()));

    connect(m_dbusInterface, SIGNAL(deviceAdded(QString)),
            this, SLOT(deviceAdded(QString)));
    connect(m_dbusInterface, SIGNAL(deviceVisibilityChanged(QString,bool)),
            this, SLOT(deviceStatusChanged(QString)));
    connect(m_dbusInterface, SIGNAL(deviceRemoved(QString)),
            this, SLOT(deviceRemoved(QString)));

    refreshDeviceList();

    //Role names for QML
    QHash<int, QByteArray> names = roleNames();
    names.insert(IdModelRole, "deviceId");
    setRoleNames(names);

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
DevicesModel::StatusFlags DevicesModel::displayFilter() const
{
    return m_displayFilter;
}

void DevicesModel::setDisplayFilter(int flags)
{
    setDisplayFilter((StatusFlags)flags);
}

void DevicesModel::setDisplayFilter(DevicesModel::StatusFlags flags)
{
    m_displayFilter = flags;
    refreshDeviceList();
}

void DevicesModel::refreshDeviceList()
{

    if (!m_deviceList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_deviceList.size() - 1);
        m_deviceList.clear();
        endRemoveRows();
    }

    if (!m_dbusInterface->isValid()) {
        return;
    }

    QDBusPendingReply<QStringList> pendingDeviceIds = m_dbusInterface->devices();
    pendingDeviceIds.waitForFinished();
    if (pendingDeviceIds.isError()) return;
    const QStringList& deviceIds = pendingDeviceIds.value();

    Q_FOREACH(const QString& id, deviceIds) {

        DeviceDbusInterface* deviceDbusInterface = new DeviceDbusInterface(id,this);

        bool onlyPaired = (m_displayFilter & StatusPaired);
        if (onlyPaired && !deviceDbusInterface->isPaired()) continue;
        bool onlyReachable = (m_displayFilter & StatusReachable);
        if (onlyReachable && !deviceDbusInterface->isReachable()) continue;

        int firstRow = m_deviceList.size();
        int lastRow = firstRow;

        beginInsertRows(QModelIndex(), firstRow, lastRow);
        m_deviceList.append(deviceDbusInterface);
        endInsertRows();

    }

    Q_EMIT dataChanged(index(0), index(m_deviceList.size()));

}

QVariant DevicesModel::data(const QModelIndex &index, int role) const
{
    if (!m_dbusInterface->isValid()
        || !index.isValid()
        || index.row() < 0
        || index.row() >= m_deviceList.size()
        || !m_deviceList[index.row()]->isValid())
    {
        return QVariant();
    }

    DeviceDbusInterface* device = m_deviceList[index.row()];

    //FIXME: This function gets called lots of times, producing lots of dbus calls. Add a cache.
    switch (role) {
        case IconModelRole: {
            bool paired = device->isPaired();
            bool reachable = device->isReachable();
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
            if (device->isReachable()) {
                status |= StatusReachable;
                if (device->isPaired()) status |= StatusPaired;
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
    if(parent.isValid()) {
        //Return size 0 if we are a child because this is not a tree
        return 0;
    }

    return m_deviceList.size();
}

