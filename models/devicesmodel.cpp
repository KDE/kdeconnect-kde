/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "devicesmodel.h"
#include "models_debug.h"

#include <KLocalizedString>

#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QIcon>
#include <QString>

#include "dbusinterfaces.h"
#include <dbushelper.h>

DevicesModel::DevicesModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(new DaemonDbusInterface(this))
    , m_displayFilter(StatusFilterFlag::NoFilter)
{
    connect(this, &QAbstractItemModel::rowsRemoved, this, &DevicesModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsInserted, this, &DevicesModel::rowsChanged);

    connect(m_dbusInterface, &OrgKdeKdeconnectDaemonInterface::deviceAdded, this, &DevicesModel::deviceAdded);
    connect(m_dbusInterface, &OrgKdeKdeconnectDaemonInterface::deviceVisibilityChanged, this, &DevicesModel::deviceUpdated);
    connect(m_dbusInterface, &OrgKdeKdeconnectDaemonInterface::deviceRemoved, this, &DevicesModel::deviceRemoved);

    QDBusServiceWatcher *watcher =
        new QDBusServiceWatcher(DaemonDbusInterface::activatedService(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &DevicesModel::refreshDeviceList);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &DevicesModel::clearDevices);

    setDisplayFilter(NoFilter);
}

QHash<int, QByteArray> DevicesModel::roleNames() const
{
    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
    names.insert(NameModelRole, "name");
    names.insert(IdModelRole, "deviceId");
    names.insert(IconNameRole, "iconName");
    names.insert(DeviceRole, "device");
    names.insert(StatusModelRole, "status");
    return names;
}

DevicesModel::~DevicesModel()
{
}

int DevicesModel::rowForDevice(const QString &id) const
{
    for (int i = 0, c = m_deviceList.size(); i < c; ++i) {
        if (m_deviceList[i]->id() == id) {
            return i;
        }
    }
    return -1;
}

void DevicesModel::deviceAdded(const QString &id)
{
    if (rowForDevice(id) >= 0) {
        Q_ASSERT_X(false, "deviceAdded", "Trying to add a device twice");
        return;
    }

    DeviceDbusInterface *dev = new DeviceDbusInterface(id, this);
    Q_ASSERT(dev->isValid());

    if (!passesFilter(dev)) {
        delete dev;
        return;
    }

    beginInsertRows(QModelIndex(), m_deviceList.size(), m_deviceList.size());
    appendDevice(dev);
    endInsertRows();
}

void DevicesModel::deviceRemoved(const QString &id)
{
    int row = rowForDevice(id);
    if (row >= 0) {
        beginRemoveRows(QModelIndex(), row, row);
        delete m_deviceList.takeAt(row);
        endRemoveRows();
    }
}

void DevicesModel::deviceUpdated(const QString &id)
{
    int row = rowForDevice(id);

    if (row < 0) {
        // FIXME: when m_dbusInterface is not valid refreshDeviceList() does
        // nothing and we can miss some devices.
        // Someone can reproduce this problem by restarting kdeconnectd while
        // kdeconnect's plasmoid is still running.
        // Another reason for this branch is that we removed the device previously
        // because of the filter settings.
        qCDebug(KDECONNECT_MODELS) << "Adding missing or previously removed device" << id;
        deviceAdded(id);
    } else {
        DeviceDbusInterface *dev = getDevice(row);
        if (!passesFilter(dev)) {
            beginRemoveRows(QModelIndex(), row, row);
            delete m_deviceList.takeAt(row);
            endRemoveRows();
            qCDebug(KDECONNECT_MODELS) << "Removed changed device " << id;
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

    refreshDeviceList();

    Q_EMIT displayFilterChanged(flags);
}

void DevicesModel::refreshDeviceList()
{
    if (!m_dbusInterface->isValid()) {
        clearDevices();
        qCWarning(KDECONNECT_MODELS) << "dbus interface not valid";
        return;
    }

    bool onlyPaired = (m_displayFilter & StatusFilterFlag::Paired);
    bool onlyReachable = (m_displayFilter & StatusFilterFlag::Reachable);

    QDBusPendingReply<QStringList> pendingDeviceIds = m_dbusInterface->devices(onlyReachable, onlyPaired);
    QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pendingDeviceIds, this);

    QObject::connect(watcher, &QDBusPendingCallWatcher::finished, this, &DevicesModel::receivedDeviceList);
}

void DevicesModel::receivedDeviceList(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    clearDevices();
    QDBusPendingReply<QStringList> pendingDeviceIds = *watcher;
    if (pendingDeviceIds.isError()) {
        qCWarning(KDECONNECT_MODELS) << "error while refreshing device list" << pendingDeviceIds.error().message();
        return;
    }

    Q_ASSERT(m_deviceList.isEmpty());
    const QStringList deviceIds = pendingDeviceIds.value();

    if (deviceIds.isEmpty())
        return;

    beginInsertRows(QModelIndex(), 0, deviceIds.count() - 1);
    for (const QString &id : deviceIds) {
        appendDevice(new DeviceDbusInterface(id, this));
    }
    endInsertRows();
}

void DevicesModel::appendDevice(DeviceDbusInterface *dev)
{
    m_deviceList.append(dev);
    connect(dev, &OrgKdeKdeconnectDeviceInterface::nameChanged, this, [this, dev]() {
        Q_ASSERT(rowForDevice(dev->id()) >= 0);
        deviceUpdated(dev->id());
    });
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

QVariant DevicesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_deviceList.size()) {
        return QVariant();
    }

    Q_ASSERT(m_dbusInterface->isValid());

    DeviceDbusInterface *device = m_deviceList[index.row()];
    Q_ASSERT(device->isValid());

    // This function gets called lots of times, producing lots of dbus calls. Add a cache?
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
        bool trusted = device->isPaired();
        bool reachable = device->isReachable();
        QString status = reachable ? (trusted ? i18n("Device trusted and connected") : i18n("Device not trusted")) : i18n("Device disconnected");
        return status;
    }
    case StatusModelRole: {
        int status = StatusFilterFlag::NoFilter;
        if (device->isReachable()) {
            status |= StatusFilterFlag::Reachable;
        }
        if (device->isPaired()) {
            status |= StatusFilterFlag::Paired;
        }
        return status;
    }
    case IconNameRole:
        return device->statusIconName();
    case DeviceRole:
        return QVariant::fromValue<QObject *>(device);
    default:
        return QVariant();
    }
}

DeviceDbusInterface *DevicesModel::deviceForId(const QString &id) const
{
    for (auto dev : m_deviceList) {
        if (dev->id() == id) {
            return dev;
        }
    }
    return nullptr;
}

DeviceDbusInterface *DevicesModel::getDevice(int row) const
{
    if (row < 0 || row >= m_deviceList.size()) {
        return nullptr;
    }

    return m_deviceList[row];
}

int DevicesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // Return size 0 if we are a child because this is not a tree
        return 0;
    }

    return m_deviceList.size();
}

bool DevicesModel::passesFilter(DeviceDbusInterface *dev) const
{
    bool onlyPaired = (m_displayFilter & StatusFilterFlag::Paired);
    bool onlyReachable = (m_displayFilter & StatusFilterFlag::Reachable);

    return !((onlyReachable && !dev->isReachable()) || (onlyPaired && !dev->isPaired()));
}

#include "moc_devicesmodel.cpp"
