/**
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef REMOTECOMMANDSMODEL_H
#define REMOTECOMMANDSMODEL_H

#include <QAbstractListModel>

#include "dbusinterfaces.h"

#include "kdeconnectmodels_export.h"

struct Command {
    QString key;
    QString name;
    QString command;
};

class KDECONNECTMODELS_EXPORT RemoteCommandsModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)

public:
    enum ModelRoles {
        KeyRole,
        NameRole,
        CommandRole,
    };

    explicit RemoteCommandsModel(QObject *parent = nullptr);
    ~RemoteCommandsModel() override;

    QString deviceId() const;
    void setDeviceId(const QString &deviceId);

    QVariant data(const QModelIndex &index, int role) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void refreshCommandList();
    void clearCommands();

Q_SIGNALS:
    void deviceIdChanged(const QString &value);
    void rowsChanged();

private:
    RemoteCommandsDbusInterface *m_dbusInterface;
    QVector<Command> m_commandList;
    QString m_deviceId;
};

#endif // DEVICESMODEL_H
