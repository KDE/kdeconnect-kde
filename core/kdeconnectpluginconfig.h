/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECTPLUGINCONFIG_H
#define KDECONNECTPLUGINCONFIG_H

#include <QDir>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "kdeconnectcore_export.h"
#include <memory>

struct KdeConnectPluginConfigPrivate;

class KDECONNECTCORE_EXPORT KdeConnectPluginConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString deviceId READ deviceId WRITE setDeviceId NOTIFY deviceIdChanged)
    Q_PROPERTY(QString pluginName READ pluginName WRITE setPluginName NOTIFY configChanged)

public:
    explicit KdeConnectPluginConfig(QObject *parent = nullptr);
    explicit KdeConnectPluginConfig(const QString &deviceId, const QString &pluginName, QObject *parent = nullptr);
    ~KdeConnectPluginConfig() override;

    /**
     * Store a key-value pair in this config object
     */
    Q_SCRIPTABLE void set(const QString &key, const QVariant &value);

    /**
     * Store a list of values in this config object under the array name
     * specified in key.
     */
    void setList(const QString &key, const QVariantList &list);

    /**
     * Read a key-value pair from this config object
     */
    Q_SCRIPTABLE QString getString(const QString &key, const QString &defaultValue);
    Q_SCRIPTABLE bool getBool(const QString &key, const bool defaultValue);
    Q_SCRIPTABLE int getInt(const QString &key, const int defaultValue);
    Q_SCRIPTABLE QByteArray getByteArray(const QString &key, const QByteArray defaultValue);

    QVariantList getList(const QString &key, const QVariantList &defaultValue = {});

    QString deviceId();
    void setDeviceId(const QString &deviceId);

    QString pluginName();
    void setPluginName(const QString &pluginName);

private Q_SLOTS:
    void slotConfigChanged();

Q_SIGNALS:
    void configChanged();
    void deviceIdChanged(const QString &value);

private:
    void loadConfig();

    std::unique_ptr<KdeConnectPluginConfigPrivate> d;
    QString m_deviceId;
    QString m_pluginName;
};

#endif
