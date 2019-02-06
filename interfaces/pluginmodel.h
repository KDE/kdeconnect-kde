/**
 * Copyright 2019 Nicolas Fella <nicolas.fella@gmx.de>
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

#ifndef PLUGINMODEL
#define PLUGINMODEL

#include <QAbstractListModel>

#include <KPluginInfo>

#include "interfaces/dbusinterfaces.h"
#include <KSharedConfig>

class KDECONNECTINTERFACES_EXPORT PluginModel
    : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId CONSTANT)

public:

    enum ExtraRoles {
        NameRole,
        CheckedRole,
        IconRole,
        IdRole,
        ConfigSourceRole
    };

    Q_ENUM(ExtraRoles)

    explicit PluginModel(QObject *parent = nullptr);
    ~PluginModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setDeviceId(const QString& deviceId);
    QString deviceId();

Q_SIGNALS:
    void deviceIdChanged(const QString& value);
    void rowsChanged();


private:
    QList<KPluginInfo> m_plugins;
    QString m_deviceId;
    KSharedConfigPtr m_config;

};


#endif // KPLUGINSELECTOR_P_H


