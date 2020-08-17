/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECTPLUGINCONFIG_H
#define KDECONNECTPLUGINCONFIG_H

#include <QObject>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "kdeconnectcore_export.h"

struct KdeConnectPluginConfigPrivate;

class KDECONNECTCORE_EXPORT KdeConnectPluginConfig : public QObject
{
    Q_OBJECT

public:
    KdeConnectPluginConfig(const QString& deviceId, const QString& pluginName);
    ~KdeConnectPluginConfig() override;

    /**
     * Store a key-value pair in this config object
     */
    void set(const QString& key, const QVariant& value);

    /**
     * Store a list of values in this config object under the array name
     * specified in key.
     */
    void setList(const QString& key, const QVariantList& list);

    /**
     * Read a key-value pair from this config object
     */
    QVariant get(const QString& key, const QVariant& defaultValue);

    /**
     * Convenience method that will convert the QVariant to whatever type for you
     */
    template<typename T> T get(const QString& key, const T& defaultValue = {}) {
        return get(key, QVariant(defaultValue)).template value<T>(); //Important note: Awesome template syntax is awesome
    }

    QVariantList getList(const QString& key, const QVariantList& defaultValue = {});

private Q_SLOTS:
    void slotConfigChanged();

Q_SIGNALS:
    void configChanged();

private:
    QScopedPointer<KdeConnectPluginConfigPrivate> d;
};

#endif
