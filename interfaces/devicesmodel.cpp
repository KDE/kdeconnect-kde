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
#include <QDBusServiceWatcher>

#include "dbusinterfaces.h"
// #include "modeltest.h"

Q_LOGGING_CATEGORY(KDECONNECT_INTERFACES, "kdeconnect.interfaces");

static QString createId() { return QCoreApplication::instance()->applicationName()+QString::number(QCoreApplication::applicationPid()); }

Q_GLOBAL_STATIC_WITH_ARGS(QString, s_keyId, (createId()));

DevicesModel::DevicesModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(new DaemonDbusInterface(this))
    , m_displayFilter(StatusFilterFlag::NoFilter)
{

    //new ModelTest(this, this);

    connect(this, &QAbstractItemModel::rowsRemoved,
            this, &DevicesModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsInserted,
            this, &DevicesModel::rowsChanged);

    connect(m_dbusInterface, SIGNAL(deviceAdded(QString)),
            this, SLOT(deviceAdded(QString)));
    connect(m_dbusInterface, &OrgKdeKdeconnectDaemonInterface::deviceVisibilityChanged,
            this, &DevicesModel::deviceUpdated);
    connect(m_dbusInterface, &OrgKdeKdeconnectDaemonInterface::deviceRemoved,
            this, &DevicesModel::deviceRemoved);

    QDBusServiceWatcher* watcher = new QDBusServiceWatcher(DaemonDbusInterface::activatedService(),
                                                           QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &DevicesModel::refreshDeviceList);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &DevicesModel::clearDevices);

    //refresh the view, acquireDiscoveryMode if necessary
    setDisplayFilter(NoFilter);
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
    m_dbusInterface->releaseDiscoveryMode(*s_keyId);
}

int DevicesModel::rowForDevice(const QString& id) const
{
    for (int i = 0, c=m_deviceList.size(); i<c; ++i) {
        if (m_deviceList[i]->id() == id) {
            return i;
        }
    }
    return -1;
}

void DevicesModel::deviceAdded(const QString& id)
{
    if (rowForDevice(id) >= 0) {
        Q_ASSERT_X(false, "deviceAdded", "Trying to add a device twice");
        return;
    }

    DeviceDbusInterface* dev = new DeviceDbusInterface(id, this);
    Q_ASSERT(dev->isValid());

    if (! passesFilter(dev)) {
        delete dev;
        return;
    }

    beginInsertRows(QModelIndex(), m_deviceList.size(), m_deviceList.size());
    appendDevice(dev);
    endInsertRows();
}

void DevicesModel::deviceRemoved(const QString& id)
{
    int row = rowForDevice(id);
    if (row>=0) {
        beginRemoveRows(QModelIndex(), row, row);
        delete m_deviceList.takeAt(row);
        endRemoveRows();
    }
}

void DevicesModel::deviceUpdated(const QString& id, bool isVisible)
{
    Q_UNUSED(isVisible);
    int row = rowForDevice(id);

    if (row < 0) {
        // FIXME: when m_dbusInterface is not valid refreshDeviceList() does
        // nothing and we can miss some devices.
        // Someone can reproduce this problem by restarting kdeconnectd while
        // kdeconnect's plasmoid is still running.
        // Another reason for this branch is that we removed the device previously
        // because of the filter settings.
        qCDebug(KDECONNECT_INTERFACES) << "Adding missing or previously removed device" << id;
        deviceAdded(id);
    } else {
        DeviceDbusInterface* dev = getDevice(row);
        if (! passesFilter(dev)) {
            beginRemoveRows(QModelIndex(), row, row);
            delete m_deviceList.takeAt(row);
            endRemoveRows();
            qCDebug(KDECONNECT_INTERFACES) << "Removed changed device " << id;
        } else {
            const QModelIndex idx = index(row);
            Q_EMIT dataChanged(idx, idx);
        }
    }
}

int DevicesModel::displayFilter() const
{
    return m_displayFilter;
}

void DevicesModel::setDisplayFilter(int flags)
{
    m_displayFilter = (StatusFilterFlag)flags;

    const bool reachableNeeded = (m_displayFilter & StatusFilterFlag::Reachable);
    if (reachableNeeded)
        m_dbusInterface->acquireDiscoveryMode(*s_keyId);
    else
        m_dbusInterface->releaseDiscoveryMode(*s_keyId);

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
    QDBusPendingCallWatcher* watcher = new QDBusPendingCallWatcher(pendingDeviceIds, this);

    QObject::connect(watcher, &QDBusPendingCallWatcher::finished,
                     this, &DevicesModel::receivedDeviceList);
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
    const QStringList deviceIds = pendingDeviceIds.value();

    if (deviceIds.isEmpty())
        return;

    beginInsertRows(QModelIndex(), 0, deviceIds.count()-1);
    for (const QString& id : deviceIds) {
        appendDevice(new DeviceDbusInterface(id, this));
    }
    endInsertRows();
}

void DevicesModel::appendDevice(DeviceDbusInterface* dev)
{
    m_deviceList.append(dev);
    connect(dev, &OrgKdeKdeconnectDeviceInterface::nameChanged, this, &DevicesModel::nameChanged);
}

void DevicesModel::nameChanged(const QString& newName)
{
    Q_UNUSED(newName);
    DeviceDbusInterface* device = static_cast<DeviceDbusInterface*>(sender());

    Q_ASSERT(rowForDevice(device->id()) >= 0);

    deviceUpdated(device->id(), true);
}

void DevicesModel::clearDevices()
{
    if (!m_deviceList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_deviceList.size() - 1);
        qDeleteAll(m_deviceList);
        m_deviceList.clear();
        endRemoveRows();
    }
}

QVariant DevicesModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= m_deviceList.size())
    {
        return QVariant();
    }

    Q_ASSERT(m_dbusInterface->isValid());

    DeviceDbusInterface* device = m_deviceList[index.row()];
    Q_ASSERT(device->isValid());

    //This function gets called lots of times, producing lots of dbus calls. Add a cache?
    switch (role) {
        case Qt::SizeHintRole:
            return QSize(0, 32);
        case IconModelRole: {
            QString icon = data(index, IconNameRole).toString();
            return QIcon::fromTheme(icon);
        }
        case IdModelRole:
            return device->id();
        case NameModelRole:
            return device->name();
        case Qt::ToolTipRole: {
            bool trusted = device->isTrusted();
            bool reachable = device->isReachable();
            QString status = reachable? (trusted? i18n("Device trusted and connected") : i18n("Device not trusted")) : i18n("Device disconnected");
            return status;
        }
        case StatusModelRole: {
            int status = StatusFilterFlag::NoFilter;
            if (device->isReachable()) {
                status |= StatusFilterFlag::Reachable;
            }
            if (device->isTrusted()) {
                status |= StatusFilterFlag::Paired;
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
        return nullptr;
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

bool DevicesModel::passesFilter(DeviceDbusInterface* dev) const
{
    bool onlyPaired = (m_displayFilter & StatusFilterFlag::Paired);
    bool onlyReachable = (m_displayFilter & StatusFilterFlag::Reachable);

    return !((onlyReachable && !dev->isReachable()) || (onlyPaired && !dev->isTrusted()));
}
