/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef NOTIFICATIONSMODEL_H
#define NOTIFICATIONSMODEL_H

#include <QAbstractItemModel>
#include <QAbstractListModel>
#include <QList>
#include <QPixmap>

#include "dbusinterfaces.h"

#include "kdeconnectmodels_export.h"

class KDECONNECTMODELS_EXPORT NotificationsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY rowsChanged)
    Q_PROPERTY(bool isAnyDimissable READ isAnyDimissable NOTIFY anyDismissableChanged STORED false)

public:
    enum ModelRoles {
        IconModelRole = Qt::DecorationRole,
        NameModelRole = Qt::DisplayRole,
        ContentModelRole = Qt::UserRole,
        AppNameModelRole = Qt::UserRole + 1,
        IdModelRole,
        DismissableModelRole,
        RepliableModelRole,
        IconPathModelRole,
        DbusInterfaceRole,
        TitleModelRole,
        TextModelRole
    };

    explicit NotificationsModel(QObject *parent = nullptr);
    ~NotificationsModel() override;

    QString deviceId() const;
    void setDeviceId(const QString &deviceId);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    NotificationDbusInterface *getNotification(const QModelIndex &index) const;

    Q_INVOKABLE bool isAnyDimissable() const;
    QHash<int, QByteArray> roleNames() const override;

public Q_SLOTS:
    void dismissAll();

private Q_SLOTS:
    void notificationAdded(const QString &id);
    void notificationRemoved(const QString &id);
    void notificationUpdated();
    void refreshNotificationList();
    void receivedNotifications(QDBusPendingCallWatcher *watcher);
    void clearNotifications();

Q_SIGNALS:
    void deviceIdChanged(const QString &value);
    void anyDismissableChanged();
    void rowsChanged();

private:
    DeviceNotificationsDbusInterface *m_dbusInterface;
    QList<NotificationDbusInterface *> m_notificationList;
    QString m_deviceId;
};

#endif // DEVICESMODEL_H
