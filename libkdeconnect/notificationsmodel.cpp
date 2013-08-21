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

#include "notificationsmodel.h"
#include <ksharedconfig.h>

#include <QDebug>
#include <QDBusInterface>

#include <KConfigGroup>
#include <KIcon>

NotificationsModel::NotificationsModel(const QString& deviceId, QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(new DeviceNotificationsDbusInterface(deviceId, this))
    , m_deviceId(deviceId)
{

    connect(m_dbusInterface, SIGNAL(notificationPosted(QString)),
            this, SLOT(notificationAdded(QString)));
    connect(m_dbusInterface, SIGNAL(notificationRemoved(QString)),
            this, SLOT(notificationRemoved(QString)));

    refreshNotificationList();

}

NotificationsModel::~NotificationsModel()
{
}

void NotificationsModel::notificationAdded(const QString& id)
{
    //TODO: Actually add instead of refresh
    Q_UNUSED(id);
    refreshNotificationList();
}

void NotificationsModel::notificationRemoved(const QString& id)
{
    //TODO: Actually remove instead of refresh
    Q_UNUSED(id);
    refreshNotificationList();
}

void NotificationsModel::refreshNotificationList()
{
    if (m_notificationList.count() > 0) {
        beginRemoveRows(QModelIndex(), 0, m_notificationList.size() - 1);
        m_notificationList.clear();
        endRemoveRows();
    }

    QList<QString> notificationIds = m_dbusInterface->activeNotifications();
    beginInsertRows(QModelIndex(), 0, notificationIds.size()-1);
    Q_FOREACH(const QString& notificationId, notificationIds) {
        NotificationDbusInterface* dbusInterface = new NotificationDbusInterface(m_deviceId,notificationId,this);
        m_notificationList.append(dbusInterface);
    }
    endInsertRows();


    Q_EMIT dataChanged(index(0), index(notificationIds.count()));

}

QVariant NotificationsModel::data(const QModelIndex &index, int role) const
{
    if (!m_dbusInterface->isValid()) {
        switch (role) {
            case IconModelRole:
                return KIcon("dialog-close").pixmap(32, 32);
            case NameModelRole:
                return QString("KDED not running");
            default:
                return QVariant();
        }
    }

    if (!index.isValid()
        || index.row() < 0
        || index.row() >= m_notificationList.count()
        || !m_notificationList[index.row()]->isValid())
    {
        return QVariant();
    }

    NotificationDbusInterface* notification = m_notificationList[index.row()];

    //FIXME: This function gets called lots of times, producing lots of dbus calls. Add a cache.
    switch (role) {
        case IconModelRole:
            return KIcon("device-notifier").pixmap(32, 32);
        case IdModelRole:
            return QString(notification->internalId());
        case NameModelRole:
            return QString(notification->ticker());
        case Qt::ToolTipRole:
            return QVariant(); //To implement
        case ContentModelRole: {
            return QString("AAAAAA"); //To implement
        }
        default:
             return QVariant();
    }
}

NotificationDbusInterface* NotificationsModel::getNotification(const QModelIndex& index)
{
    if (!index.isValid()) {
        return NULL;
    }

    int row = index.row();
    if (row < 0 || row >= m_notificationList.size()) {
        return NULL;
    }

    return m_notificationList[row];
}

int NotificationsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);

    return m_notificationList.count();
}

