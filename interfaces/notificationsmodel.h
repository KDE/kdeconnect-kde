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

#ifndef NOTIFICATIONSMODEL_H
#define NOTIFICATIONSMODEL_H

#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QPixmap>
#include <QList>

#include "interfaces/dbusinterfaces.h"

class KDECONNECTINTERFACES_EXPORT NotificationsModel
    : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowsChanged)
    Q_PROPERTY(bool isAnyDimissable READ isAnyDimissable NOTIFY anyDismissableChanged STORED false)

public:
    enum ModelRoles {
        IconModelRole      = Qt::DecorationRole,
        NameModelRole      = Qt::DisplayRole,
        ContentModelRole   = Qt::UserRole,
        AppNameModelRole   = Qt::UserRole + 1,
        IdModelRole,
        DismissableModelRole,
        RepliableModelRole,
        IconPathModelRole,
        DbusInterfaceRole,
        TitleModelRole,
        TextModelRole
    };

    explicit NotificationsModel(QObject* parent = nullptr);
    ~NotificationsModel() override;

    QString deviceId() const;
    void setDeviceId(const QString& deviceId);

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    NotificationDbusInterface* getNotification(const QModelIndex& index) const;

    Q_INVOKABLE bool isAnyDimissable() const;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void dismissAll();

private Q_SLOTS:
    void notificationAdded(const QString& id);
    void notificationRemoved(const QString& id);
    void notificationUpdated(const QString& id);
    void refreshNotificationList();
    void receivedNotifications(QDBusPendingCallWatcher* watcher);
    void clearNotifications();

Q_SIGNALS:
    void deviceIdChanged(const QString& value);
    void anyDismissableChanged();
    void rowsChanged();

private:
    DeviceNotificationsDbusInterface* m_dbusInterface;
    QList<NotificationDbusInterface*> m_notificationList;
    QString m_deviceId;
};

#endif // DEVICESMODEL_H
