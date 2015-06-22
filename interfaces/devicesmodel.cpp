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
#include "interfaces_debug.h"

#include <KLocalizedString>

#include <QString>
#include <QDBusInterface>
#include <QDBusPendingReply>
#include <QIcon>

#include "dbusinterfaces.h"
// #include "modeltest.h"

Q_LOGGING_CATEGORY(KDECONNECT_INTERFACES, "kdeconnect.interfaces");

DevicesModel::DevicesModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(new DaemonDbusInterface(this))
    , m_displayFilter(StatusFilterFlag::NoFilter)
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
}

QHash< int, QByteArray > DevicesModel::roleNames() const
{
    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
    names.insert(IdModelRole, "deviceId");
    names.insert(IconNameRole, "iconName");
    names.insert(DeviceRole, "device");
    names.insert(StatusModelRole, "status");
    return names;
}

DevicesModel::~DevicesModel()
{
}

void DevicesModel::deviceAdded(const QString& id)
{
    if (m_deviceIndexById.contains(id)) {
        Q_ASSERT(false); //This should only happen for new devices
        return;
    }

    DeviceDbusInterface* dev = new DeviceDbusInterface(id, this);

    bool onlyPaired = (m_displayFilter & StatusFilterFlag::Paired);
    bool onlyReachable = (m_displayFilter & StatusFilterFlag::Reachable);

    if ((onlyReachable && !dev->isReachable()) || (onlyPaired && !dev->isPaired())) {
        return;
    }

    beginInsertRows(QModelIndex(), m_deviceList.size(), m_deviceList.size());
    appendDevice(dev);
    endInsertRows();
}

void DevicesModel::deviceRemoved(const QString& id)
{
    if (!m_deviceIndexById.contains(id)) {
        Q_ASSERT(false); //This should only be emited for existing devices
        return;
    }
    const int row = m_deviceIndexById.take(id);
    beginRemoveRows(QModelIndex(), row, row);
    delete m_deviceList.takeAt(row);
    endRemoveRows();
}

void DevicesModel::deviceStatusChanged(const QString& id)
{
    if (!m_deviceIndexById.contains(id)) {
        Q_ASSERT(false); //This should only be emited for existing devices
        return;
    }

    int row = m_deviceIndexById[id];
    const QModelIndex idx = index(row);
    Q_EMIT dataChanged(idx, idx);
}

int DevicesModel::displayFilter() const
{
    return m_displayFilter;
}

void DevicesModel::setDisplayFilter(int flags)
{
    m_displayFilter = (StatusFilterFlag)flags;
    refreshDeviceList();
}

void DevicesModel::refreshDeviceList()
{
    if (!m_dbusInterface->isValid()) {
        clearDevices();
        qCWarning(KDECONNECT_INTERFACES) << "dbus interface not valid";
        return;
    }

    bool onlyPaired = (m_displayFilter & StatusFilterFlag::Paired);
    bool onlyReachable = (m_displayFilter & StatusFilterFlag::Reachable);

    QDBusPendingReply<QStringList> pendingDeviceIds = m_dbusInterface->devices(onlyReachable, onlyPaired);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingDeviceIds, this);

    QObject::connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                     this, SLOT(receivedDeviceList(QDBusPendingCallWatcher*)));
}

void DevicesModel::receivedDeviceList(QDBusPendingCallWatcher* watcher)
{
    watcher->deleteLater();
    clearDevices();
    QDBusPendingReply<QStringList> pendingDeviceIds = *watcher;
    if (pendingDeviceIds.isError()) {
        qCWarning(KDECONNECT_INTERFACES) << "error while refreshing device list" << pendingDeviceIds.error().message();
        return;
    }

    Q_ASSERT(m_deviceList.isEmpty());
    Q_ASSERT(m_deviceIndexById.isEmpty());
    const QStringList deviceIds = pendingDeviceIds.value();

    if (deviceIds.isEmpty())
        return;

    beginInsertRows(QModelIndex(), 0, deviceIds.count()-1);
    Q_FOREACH(const QString& id, deviceIds) {
        appendDevice(new DeviceDbusInterface(id, this));
    }
    endInsertRows();
}

void DevicesModel::appendDevice(DeviceDbusInterface* dev)
{
    m_deviceIndexById.insert(dev->id(), m_deviceList.size());
    m_deviceList.append(dev);
    connect(dev, SIGNAL(nameChanged(QString)), SLOT(nameChanged(QString)));
}

void DevicesModel::nameChanged(const QString& newName)
{
    Q_UNUSED(newName);

    DeviceDbusInterface* device = static_cast<DeviceDbusInterface*>(sender());

    int row = m_deviceIndexById[device->id()];
    Q_ASSERT(row >= 0);
    const QModelIndex idx = index(row);
    Q_EMIT dataChanged(idx, idx);
}

void DevicesModel::clearDevices()
{
    if (!m_deviceList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_deviceList.size() - 1);
        qDeleteAll(m_deviceList);
        m_deviceList.clear();
        m_deviceIndexById.clear();
        endRemoveRows();
    }
}

QVariant DevicesModel::data(const QModelIndex& index, int role) const
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

    //This function gets called lots of times, producing lots of dbus calls. Add a cache?
    switch (role) {
        case IconModelRole: {
            QString icon = data(index, IconNameRole).toString();
            return QIcon::fromTheme(icon);
        }
        case IdModelRole:
            return device->id();
        case NameModelRole:
            return device->name();
        case Qt::ToolTipRole: {
            bool paired = device->isPaired();
            bool reachable = device->isReachable();
            QString status = reachable? (paired? i18n("Device trusted and connected") : i18n("Device not trusted")) : i18n("Device disconnected");
            return status;
        }
        case StatusModelRole: {
            int status = StatusFilterFlag::NoFilter;
            if (device->isReachable()) {
                status |= StatusFilterFlag::Reachable;
                if (device->isPaired()) status |= StatusFilterFlag::Paired;
            }
            return status;
        }
        case IconNameRole:
            return device->statusIconName();
        case DeviceRole:
            return QVariant::fromValue<QObject*>(device);
        default:
            return QVariant();
    }
}

DeviceDbusInterface* DevicesModel::getDevice(int row) const
{
    if (row < 0 || row >= m_deviceList.size()) {
        return NULL;
    }

    return m_deviceList[row];
}

int DevicesModel::rowCount(const QModelIndex& parent) const
{
    if(parent.isValid()) {
        //Return size 0 if we are a child because this is not a tree
        return 0;
    }

    return m_deviceList.size();
}
