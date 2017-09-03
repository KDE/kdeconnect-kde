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

#ifndef DEVICESMODEL_H
#define DEVICESMODEL_H

#include <QAbstractListModel>
#include <QPixmap>
#include <QList>

#include "interfaces/kdeconnectinterfaces_export.h"

class QDBusPendingCallWatcher;
class DaemonDbusInterface;
class DeviceDbusInterface;

class KDECONNECTINTERFACES_EXPORT DevicesModel
    : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int displayFilter READ displayFilter WRITE setDisplayFilter)
    Q_PROPERTY(int count READ rowCount NOTIFY rowsChanged)

public:
    enum ModelRoles {
        NameModelRole   = Qt::DisplayRole,
        IconModelRole   = Qt::DecorationRole,
        StatusModelRole = Qt::InitialSortOrderRole,
        IdModelRole     = Qt::UserRole,
        IconNameRole,
        DeviceRole
    };
    Q_ENUM(ModelRoles);

    // A device is always paired or reachable or both
    // You can combine the Paired and Reachable flags
    enum StatusFilterFlag {
        NoFilter   = 0x00,
        Paired     = 0x01, // show device only if it's paired
        Reachable  = 0x02  // show device only if it's reachable
    };
    Q_DECLARE_FLAGS(StatusFilterFlags, StatusFilterFlag)
    Q_FLAGS(StatusFilterFlags)
    Q_ENUM(StatusFilterFlag)

    explicit DevicesModel(QObject* parent = nullptr);
    ~DevicesModel() override;

    void setDisplayFilter(int flags);
    int displayFilter() const;

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    Q_SCRIPTABLE DeviceDbusInterface* getDevice(int row) const;
    QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void deviceAdded(const QString& id);
    void deviceRemoved(const QString& id);
    void deviceUpdated(const QString& id, bool isVisible);
    void refreshDeviceList();
    void receivedDeviceList(QDBusPendingCallWatcher* watcher);
    void nameChanged(const QString& newName);

Q_SIGNALS:
    void rowsChanged();

private:
    int rowForDevice(const QString& id) const;
    void clearDevices();
    void appendDevice(DeviceDbusInterface* dev);
    bool passesFilter(DeviceDbusInterface* dev) const;

    DaemonDbusInterface* m_dbusInterface;
    QVector<DeviceDbusInterface*> m_deviceList;
    StatusFilterFlag m_displayFilter;
};

//Q_DECLARE_OPERATORS_FOR_FLAGS(DevicesModel::StatusFilterFlag)

#endif // DEVICESMODEL_H
