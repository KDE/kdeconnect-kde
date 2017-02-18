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

#include "kdeconnectpluginconfig.h"

#include <QDir>
#include <QSettings>
#include <QDBusMessage>
#include <QDBusConnection>

#include "kdeconnectconfig.h"

struct KdeConnectPluginConfigPrivate
{
    QDir mConfigDir;
    QSettings* mConfig;
    QDBusMessage signal;
};

KdeConnectPluginConfig::KdeConnectPluginConfig(const QString& deviceId, const QString& pluginName)
    : d(new KdeConnectPluginConfigPrivate())
{
    d->mConfigDir = KdeConnectConfig::instance()->pluginConfigDir(deviceId, pluginName);
    QDir().mkpath(d->mConfigDir.path());

    d->mConfig = new QSettings(d->mConfigDir.absoluteFilePath(QStringLiteral("config")), QSettings::IniFormat);

    d->signal = QDBusMessage::createSignal("/kdeconnect/"+deviceId+"/"+pluginName, QStringLiteral("org.kde.kdeconnect.config"), QStringLiteral("configChanged"));
    QDBusConnection::sessionBus().connect(QLatin1String(""), "/kdeconnect/"+deviceId+"/"+pluginName, QStringLiteral("org.kde.kdeconnect.config"), QStringLiteral("configChanged"), this, SLOT(slotConfigChanged()));
}

KdeConnectPluginConfig::~KdeConnectPluginConfig()
{
    delete d->mConfig;
}

QVariant KdeConnectPluginConfig::get(const QString& key, const QVariant& defaultValue)
{
    d->mConfig->sync();
    return d->mConfig->value(key, defaultValue);
}

QVariantList KdeConnectPluginConfig::getList(const QString& key,
                                             const QVariantList& defaultValue)
{
    QVariantList list;
    d->mConfig->sync();  // note: need sync() to get recent changes signalled from other process
    int size = d->mConfig->beginReadArray(key);
    if (size < 1) {
        d->mConfig->endArray();
        return defaultValue;
    }
    for (int i = 0; i < size; ++i) {
        d->mConfig->setArrayIndex(i);
        list << d->mConfig->value(QStringLiteral("value"));
    }
    d->mConfig->endArray();
    return list;
}

void KdeConnectPluginConfig::set(const QString& key, const QVariant& value)
{
    d->mConfig->setValue(key, value);
    d->mConfig->sync();
    QDBusConnection::sessionBus().send(d->signal);
}

void KdeConnectPluginConfig::setList(const QString& key, const QVariantList& list)
{
    d->mConfig->beginWriteArray(key);
    for (int i = 0; i < list.size(); ++i) {
        d->mConfig->setArrayIndex(i);
        d->mConfig->setValue(QStringLiteral("value"), list.at(i));
    }
    d->mConfig->endArray();
    d->mConfig->sync();
    QDBusConnection::sessionBus().send(d->signal);
}

void KdeConnectPluginConfig::slotConfigChanged()
{
    Q_EMIT configChanged();
}
