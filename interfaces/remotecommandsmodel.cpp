/**
 * Copyright 2018 Nicolas Fella <nicolas.fella@gmx.de>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "remotecommandsmodel.h"
#include "interfaces_debug.h"

#include <QDebug>

#include <dbushelper.h>

RemoteCommandsModel::RemoteCommandsModel(QObject* parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(nullptr)
{

    connect(this, &QAbstractItemModel::rowsInserted,
            this, &RemoteCommandsModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsRemoved,
            this, &RemoteCommandsModel::rowsChanged);

    QDBusServiceWatcher* watcher = new QDBusServiceWatcher(DaemonDbusInterface::activatedService(),
                                                           DBusHelper::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &RemoteCommandsModel::refreshCommandList);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &RemoteCommandsModel::clearCommands);
}

QHash<int, QByteArray> RemoteCommandsModel::roleNames() const
{
    //Role names for QML
    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
    names.insert(KeyRole, "key");
    names.insert(NameRole, "name");
    names.insert(CommandRole, "command");
    return names;
}

RemoteCommandsModel::~RemoteCommandsModel()
{
}

QString RemoteCommandsModel::deviceId() const
{
    return m_deviceId;
}

void RemoteCommandsModel::setDeviceId(const QString& deviceId)
{
    m_deviceId = deviceId;

    if (m_dbusInterface) {
        delete m_dbusInterface;
    }

    m_dbusInterface = new RemoteCommandsDbusInterface(deviceId, this);

    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceRemotecommandsInterface::commandsChanged,
            this, &RemoteCommandsModel::refreshCommandList);

    refreshCommandList();

    Q_EMIT deviceIdChanged(deviceId);
}

void RemoteCommandsModel::refreshCommandList()
{
    if (!m_dbusInterface) {
        return;
    }

    clearCommands();

    if (!m_dbusInterface->isValid()) {
        qCWarning(KDECONNECT_INTERFACES) << "dbus interface not valid";
        return;
    }

    const auto cmds = QJsonDocument::fromJson(m_dbusInterface->commands()).object();

    beginResetModel();

    for (auto it = cmds.constBegin(), itEnd = cmds.constEnd(); it!=itEnd; ++it) {
        const QJsonObject cont = it->toObject();
        Command command;
        command.key = it.key();
        command.name = cont.value(QStringLiteral("name")).toString();
        command.command = cont.value(QStringLiteral("command")).toString();
        m_commandList.append(command);
    }

    endResetModel();
}

QVariant RemoteCommandsModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()
        || index.row() < 0
        || index.row() >= m_commandList.count())
    {
        return QVariant();
    }

    if (!m_dbusInterface || !m_dbusInterface->isValid()) {
        return QVariant();
    }

    Command command = m_commandList[index.row()];

    switch (role) {
        case KeyRole:
            return command.key;
        case NameRole:
            return command.name;
        case CommandRole:
            return command.command;
        default:
             return QVariant();
    }
}

int RemoteCommandsModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) {
        //Return size 0 if we are a child because this is not a tree
        return 0;
    }

    return m_commandList.count();
}

void RemoteCommandsModel::clearCommands()
{
    if (!m_commandList.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, m_commandList.size() - 1);
        m_commandList.clear();
        endRemoveRows();
    }
}
