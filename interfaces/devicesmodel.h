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

#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QPixmap>
#include <QList>

#include "interfaces/kdeconnectinterfaces_export.h"

class DaemonDbusInterface;
class DeviceDbusInterface;

class KDECONNECTINTERFACES_EXPORT DevicesModel
    : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(StatusFlags displayFilter READ displayFilter WRITE setDisplayFilter)
    Q_PROPERTY(int count READ rowCount NOTIFY rowsChanged)

public:
    enum ModelRoles {
        NameModelRole = Qt::DisplayRole,
        IconModelRole = Qt::DecorationRole,
        StatusModelRole = Qt::InitialSortOrderRole,
        IdModelRole = Qt::UserRole,
        IconNameRole
    };
    enum StatusFlag {
        StatusUnknown   = 0x00,
        StatusPaired    = 0x01,
        StatusReachable = 0x02
    };
    Q_DECLARE_FLAGS(StatusFlags, StatusFlag)

    DevicesModel(QObject *parent = 0);
    virtual ~DevicesModel();

    void setDisplayFilter(StatusFlags flags);
    void setDisplayFilter(int flags);
    StatusFlags displayFilter() const;

    virtual QVariant data(const QModelIndex& index, int role) const;
    virtual int rowCount(const QModelIndex& parent = QModelIndex()) const;

    DeviceDbusInterface* getDevice(const QModelIndex& index) const;

public Q_SLOTS:
    void deviceStatusChanged(const QString& id);

private Q_SLOTS:
    void deviceAdded(const QString& id);
    void deviceRemoved(const QString& id);
    void refreshDeviceList();

Q_SIGNALS:
    void rowsChanged();

private:
    DaemonDbusInterface* m_dbusInterface;
    QList<DeviceDbusInterface*> m_deviceList;
    StatusFlags m_displayFilter;

};

#endif // DEVICESMODEL_H
