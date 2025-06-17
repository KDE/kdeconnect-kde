/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DEVICESMODEL_H
#define DEVICESMODEL_H

#include <QAbstractListModel>
#include <QList>
#include <QPixmap>

#include "kdeconnectinterfaces_export.h"

class QDBusPendingCallWatcher;
class DaemonDbusInterface;
class DeviceDbusInterface;

class KDECONNECTINTERFACES_EXPORT DevicesModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int displayFilter READ displayFilter WRITE setDisplayFilter NOTIFY displayFilterChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowsChanged)

public:
    enum ModelRoles {
        NameModelRole = Qt::DisplayRole,
        IconModelRole = Qt::DecorationRole,
        StatusModelRole = Qt::InitialSortOrderRole,
        IdModelRole = Qt::UserRole,
        IconNameRole,
        DeviceRole
    };
    Q_ENUM(ModelRoles)

    // A device is always paired or reachable or both
    // You can combine the Paired and Reachable flags
    enum StatusFilterFlag {
        NoFilter = 0x00,
        Paired = 0x01, // show device only if it's paired
        Reachable = 0x02 // show device only if it's reachable
    };
    Q_DECLARE_FLAGS(StatusFilterFlags, StatusFilterFlag)
    Q_FLAGS(StatusFilterFlags)
    Q_ENUM(StatusFilterFlag)

    explicit DevicesModel(QObject *parent = nullptr);
    ~DevicesModel() override;

    void setDisplayFilter(int flags);
    int displayFilter() const;

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    Q_SCRIPTABLE DeviceDbusInterface *getDevice(int row) const;
    Q_SCRIPTABLE DeviceDbusInterface *deviceForId(const QString &id) const;
    QHash<int, QByteArray> roleNames() const override;
    Q_SCRIPTABLE int rowForDevice(const QString &id) const;

private Q_SLOTS:
    void deviceAdded(const QString &id);
    void deviceRemoved(const QString &id);
    void deviceUpdated(const QString &id);
    void refreshDeviceList();
    void receivedDeviceList(QDBusPendingCallWatcher *watcher);

Q_SIGNALS:
    void rowsChanged();
    void displayFilterChanged(int value);

private:
    void clearDevices();
    void appendDevice(DeviceDbusInterface *dev);
    bool passesFilter(DeviceDbusInterface *dev) const;

    DaemonDbusInterface *m_dbusInterface;
    QVector<DeviceDbusInterface *> m_deviceList;
    StatusFilterFlag m_displayFilter;
};

// Q_DECLARE_OPERATORS_FOR_FLAGS(DevicesModel::StatusFilterFlag)

#endif // DEVICESMODEL_H
