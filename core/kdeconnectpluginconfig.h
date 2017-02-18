/**
 * Copyright 2015 Albert Vaca <albertvaka@gmail.com>
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
