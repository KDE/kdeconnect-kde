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

#include <QDBusInterface>

#include <KSharedConfig>
#include <KConfigGroup>
#include <KIcon>

#include "modeltest.h"
#include "kdebugnamespace.h"

NotificationsModel::NotificationsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(0)
{

    //new ModelTest(this, this);

    connect(this, SIGNAL(rowsInserted(QModelIndex, int, int)),
            this, SIGNAL(rowsChanged()));
    connect(this, SIGNAL(rowsRemoved(QModelIndex, int, int)),
            this, SIGNAL(rowsChanged()));

    connect(this, SIGNAL(dataChanged(QModelIndex,QModelIndex)),
            this, SIGNAL(anyDismissableChanged()));

    //Role names for QML
    QHash<int, QByteArray> names = roleNames();
    names.insert(DbusInterfaceRole, "dbusInterface");
    names.insert(AppNameModelRole, "appName");
    names.insert(DismissableModelRole, "dismissable");
    setRoleNames(names);

}

NotificationsModel::~NotificationsModel()
{
}

QString NotificationsModel::deviceId()
{
    return m_deviceId;
}

void NotificationsModel::setDeviceId(const QString& deviceId)
{
    m_deviceId = deviceId;

    if (m_dbusInterface) delete m_dbusInterface;
    m_dbusInterface = new DeviceNotificationsDbusInterface(deviceId, this);

    connect(m_dbusInterface, SIGNAL(notificationPosted(QString)),
            this, SLOT(notificationAdded(QString)));
    connect(m_dbusInterface, SIGNAL(notificationRemoved(QString)),
            this, SLOT(notificationRemoved(QString)));

    refreshNotificationList();

    Q_EMIT deviceIdChanged(deviceId);
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
    if (!m_dbusInterface) return;

    if (!m_notificationList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_notificationList.size() - 1);
        qDeleteAll(m_notificationList);
        m_notificationList.clear();
        endRemoveRows();
    }

    if (!m_dbusInterface->isValid()) return;

    QDBusPendingReply<QStringList> pendingNotificationIds = m_dbusInterface->activeNotifications();
    pendingNotificationIds.waitForFinished();
    if (pendingNotificationIds.isError()) return;
    const QStringList& notificationIds = pendingNotificationIds.value();

    if (notificationIds.isEmpty()) return;

    beginInsertRows(QModelIndex(), 0, notificationIds.size()-1);
    Q_FOREACH(const QString& notificationId, notificationIds) {
        NotificationDbusInterface* dbusInterface = new NotificationDbusInterface(m_deviceId, notificationId, this);
        m_notificationList.append(dbusInterface);
    }
    endInsertRows();

    Q_EMIT dataChanged(index(0), index(notificationIds.size()-1));
}

QVariant NotificationsModel::data(const QModelIndex &index, int role) const
{

    if (!index.isValid()
        || index.row() < 0
        || index.row() >= m_notificationList.count()
        || !m_notificationList[index.row()]->isValid())
    {
        return QVariant();
    }

    if (!m_dbusInterface || !m_dbusInterface->isValid()) {
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
        case ContentModelRole:
            return QString(); //To implement in the Android side
        case AppNameModelRole:
            return QString(notification->appName());
        case DbusInterfaceRole:
            return qVariantFromValue<QObject*>(notification);
        case DismissableModelRole:
            return notification->dismissable();
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
    if(parent.isValid()) {
        //Return size 0 if we are a child because this is not a tree
        return 0;
    }

    return m_notificationList.count();
}

bool NotificationsModel::isAnyDimissable()
{
    Q_FOREACH(NotificationDbusInterface* notification, m_notificationList) {
        if (notification->dismissable()) {
            return true;
        }
    }
    return false;
}


void NotificationsModel::dismissAll()
{
    Q_FOREACH(NotificationDbusInterface* notification, m_notificationList) {
        if (notification->dismissable()) {
            notification->dismiss();
        }
    }
}
