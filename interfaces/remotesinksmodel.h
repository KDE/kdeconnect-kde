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

class KDECONNECTINTERFACES_EXPORT RemoteSinksModel
    : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)

public:
    enum ModelRoles {
        NameRole,
        DescriptionRole,
        MaxVolumeRole,
        VolumeRole,
        MutedRole
    };

    explicit RemoteSinksModel(QObject* parent = nullptr);
    ~RemoteSinksModel() override;

    QString deviceId() const;
    void setDeviceId(const QString& deviceId);

    QVariant data(const QModelIndex& index, int role) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void refreshSinkList();

Q_SIGNALS:
    void deviceIdChanged(const QString& value);
    void rowsChanged();

private:
    RemoteSystemVolumeDbusInterface* m_dbusInterface;
    QVector<Sink*> m_sinkList;
    QString m_deviceId;
};

#endif // DEVICESMODEL_H
