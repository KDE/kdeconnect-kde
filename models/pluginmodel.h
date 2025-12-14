/**
 * SPDX-FileCopyrightText: 2019 Nicolas Fella <nicolas.fella@gmx.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef PLUGINMODEL
#define PLUGINMODEL

#include <QAbstractListModel>

#include <KPluginMetaData>
#include <KSharedConfig>

#include "dbusinterfaces.h"

#include "kdeconnectmodels_export.h"

class KDECONNECTMODELS_EXPORT PluginModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId)

public:
    enum ExtraRoles {
        IconRole = Qt::UserRole + 1,
        IdRole,
        ConfigSourceRole,
        DescriptionRole,
    };

    Q_ENUM(ExtraRoles)

    explicit PluginModel(QObject *parent = nullptr);
    ~PluginModel() override;

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    void setDeviceId(const QString &deviceId);
    QString deviceId();

    Q_INVOKABLE QString pluginDisplayName(const QString &pluginId) const;
    Q_INVOKABLE QUrl pluginSource(const QString &pluginId) const;

Q_SIGNALS:
    void deviceIdChanged(const QString &value);
    void rowsChanged();

private:
    QVector<KPluginMetaData> m_plugins;
    QString m_deviceId;
    KSharedConfigPtr m_config;
};

#endif // KPLUGINSELECTOR_P_H
