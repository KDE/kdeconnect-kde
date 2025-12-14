/**
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "remotecommandsmodel.h"
#include "models_debug.h"

#include <QDebug>

#include <dbushelper.h>

RemoteCommandsModel::RemoteCommandsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_dbusInterface(nullptr)
{
    connect(this, &QAbstractItemModel::rowsInserted, this, &RemoteCommandsModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &RemoteCommandsModel::rowsChanged);

    QDBusServiceWatcher *watcher =
        new QDBusServiceWatcher(DaemonDbusInterface::activatedService(), QDBusConnection::sessionBus(), QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(watcher, &QDBusServiceWatcher::serviceRegistered, this, &RemoteCommandsModel::refreshCommandList);
    connect(watcher, &QDBusServiceWatcher::serviceUnregistered, this, &RemoteCommandsModel::clearCommands);
}

QHash<int, QByteArray> RemoteCommandsModel::roleNames() const
{
    // Role names for QML
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

void RemoteCommandsModel::setDeviceId(const QString &deviceId)
{
    m_deviceId = deviceId;

    if (m_dbusInterface) {
        delete m_dbusInterface;
    }

    m_dbusInterface = new RemoteCommandsDbusInterface(deviceId, this);

    connect(m_dbusInterface, &OrgKdeKdeconnectDeviceRemotecommandsInterface::commandsChanged, this, &RemoteCommandsModel::refreshCommandList);

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
        qCWarning(KDECONNECT_MODELS) << "dbus interface not valid";
        return;
    }

    const auto cmds = QJsonDocument::fromJson(m_dbusInterface->commands()).object();

    beginResetModel();

    for (auto it = cmds.constBegin(), itEnd = cmds.constEnd(); it != itEnd; ++it) {
        const QJsonObject cont = it->toObject();
        Command command;
        command.key = it.key();
        command.name = cont.value(QStringLiteral("name")).toString();
        command.command = cont.value(QStringLiteral("command")).toString();
        m_commandList.append(command);
    }

    endResetModel();
}

QVariant RemoteCommandsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_commandList.count()) {
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

int RemoteCommandsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // Return size 0 if we are a child because this is not a tree
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

#include "moc_remotecommandsmodel.cpp"
