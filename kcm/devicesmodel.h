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

#ifndef DEVICESMODEL_H
#define DEVICESMODEL_H


#include <QAbstractItemModel>
#include <QPixmap>
#include <QList>

class DevicesModel
    : public QAbstractItemModel
{
public:
    enum ModelRoles {
        NameModelRole = Qt::DisplayRole,
        IconModelRole = Qt::DecorationRole,
        IdModelRole = Qt::UserRole,
        StatusModelRole
    };

    enum DeviceStatus {
        Missing = 0,
        Visible,
        Connected,
        PairedMissing = 10,
        PairedVisible,
        PairedConnected,
    };

    DevicesModel(QObject *parent = 0);
    virtual ~DevicesModel();

    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
    virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
    virtual QModelIndex parent(const QModelIndex &index) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
    virtual bool insertRows(int row, int count, const QModelIndex &parent = QModelIndex());
    virtual bool removeRows(int row, int count, const QModelIndex &parent = QModelIndex());

    void loadPaired();
    void addDevice(QString id, QString name, DeviceStatus status);


private:
    struct Device {
        QString id;
        QString name;
        DeviceStatus status;
    };
    QList<Device> m_deviceList;
};

#endif // DEVICESMODEL_H
