/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef COMMANDSMODEL_H
#define COMMANDSMODEL_H

#include "core/kdeconnectpluginconfig.h"
#include "kdeconnectmodels_export.h"
#include <QAbstractListModel>

struct CommandEntry {
    QString key;
    QString name;
    QString command;
};

class KDECONNECTMODELS_EXPORT CommandsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)

public:
    enum ModelRoles {
        KeyRole,
        NameRole,
        CommandRole,
    };

    explicit CommandsModel(QObject *parent = nullptr);
    ~CommandsModel() override;

    QString deviceId() const;
    void setDeviceId(const QString &deviceId);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;

    Q_SCRIPTABLE void removeCommand(int index);
    Q_SCRIPTABLE void addCommand(const QString &name, const QString &command);
    Q_SCRIPTABLE void changeCommand(int index, const QString &name, const QString &command);

private Q_SLOTS:
    void refreshCommandList();

Q_SIGNALS:
    void deviceIdChanged(const QString &value);
    void rowsChanged();

private:
    void saveCommands();

    QVector<CommandEntry> m_commandList;
    QString m_deviceId;
    KdeConnectPluginConfig m_config;
};

#endif // DEVICESMODEL_H
