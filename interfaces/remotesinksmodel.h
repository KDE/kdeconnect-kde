/**
 * SPDX-FileCopyrightText: 2018 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef REMOTESINKSMODEL_H
#define REMOTESINKSMODEL_H

#include <QAbstractListModel>

#include "dbusinterfaces.h"

struct Sink {
    QString name;
    QString description;
    int maxVolume;
    int volume;
    bool muted;
};

class KDECONNECTINTERFACES_EXPORT RemoteSinksModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)

public:
    enum ModelRoles {
        NameRole,
        DescriptionRole,
        MaxVolumeRole,
        VolumeRole,
        MutedRole,
    };

    explicit RemoteSinksModel(QObject *parent = nullptr);
    ~RemoteSinksModel() override;

    QString deviceId() const;
    void setDeviceId(const QString &deviceId);

    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void refreshSinkList();

Q_SIGNALS:
    void deviceIdChanged(const QString &value);
    void rowsChanged();

private:
    RemoteSystemVolumeDbusInterface *m_dbusInterface;
    QVector<Sink> m_sinkList;
    QString m_deviceId;
};

#endif // DEVICESMODEL_H
