/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "commandsmodel.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QUuid>

#include <dbushelper.h>

CommandsModel::CommandsModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_config()
{
    m_config.setPluginName(QStringLiteral("kdeconnect_runcommand"));
    connect(this, &QAbstractItemModel::rowsInserted, this, &CommandsModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &CommandsModel::rowsChanged);
}

QHash<int, QByteArray> CommandsModel::roleNames() const
{
    // Role names for QML
    QHash<int, QByteArray> names = QAbstractItemModel::roleNames();
    names.insert(KeyRole, "key");
    names.insert(NameRole, "name");
    names.insert(CommandRole, "command");
    return names;
}

CommandsModel::~CommandsModel()
{
}

QString CommandsModel::deviceId() const
{
    return m_deviceId;
}

void CommandsModel::setDeviceId(const QString &deviceId)
{
    m_deviceId = deviceId;
    m_config.setDeviceId(deviceId);

    refreshCommandList();

    Q_EMIT deviceIdChanged(deviceId);
}

void CommandsModel::refreshCommandList()
{
    const auto cmds = QJsonDocument::fromJson(m_config.getByteArray(QStringLiteral("commands"), QByteArray())).object();

    beginResetModel();
    m_commandList.clear();

    for (auto it = cmds.constBegin(), itEnd = cmds.constEnd(); it != itEnd; ++it) {
        const QJsonObject cont = it->toObject();
        CommandEntry command;
        command.key = it.key();
        command.name = cont.value(QStringLiteral("name")).toString();
        command.command = cont.value(QStringLiteral("command")).toString();
        m_commandList.append(command);
    }

    endResetModel();
}

QVariant CommandsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_commandList.count()) {
        return QVariant();
    }

    CommandEntry command = m_commandList[index.row()];

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

int CommandsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        // Return size 0 if we are a child because this is not a tree
        return 0;
    }

    return m_commandList.count();
}

void CommandsModel::removeCommand(int index)
{
    beginRemoveRows(QModelIndex(), index, index);
    m_commandList.remove(index);
    endRemoveRows();
    saveCommands();
}

void CommandsModel::saveCommands()
{
    QJsonObject jsonConfig;
    for (const CommandEntry &command : m_commandList) {
        QJsonObject entry;
        entry[QStringLiteral("name")] = command.name;
        entry[QStringLiteral("command")] = command.command;
        jsonConfig[command.key] = entry;
    }
    QJsonDocument document;
    document.setObject(jsonConfig);
    m_config.set(QStringLiteral("commands"), document.toJson(QJsonDocument::Compact));
}

void CommandsModel::addCommand(const QString &name, const QString &command)
{
    CommandEntry entry;
    QString key = QUuid::createUuid().toString(QUuid::WithoutBraces);
    DBusHelper::filterNonExportableCharacters(key);
    entry.key = key;
    entry.name = name;
    entry.command = command;
    beginInsertRows(QModelIndex(), m_commandList.size(), m_commandList.size());
    m_commandList.append(entry);
    endInsertRows();
    saveCommands();
}

void CommandsModel::changeCommand(int index, const QString &name, const QString &command)
{
    m_commandList[index].name = name;
    m_commandList[index].command = command;
    saveCommands();

    auto modelIndex = createIndex(index, 0);
    Q_EMIT dataChanged(modelIndex, modelIndex);
}

#include "moc_commandsmodel.cpp"
