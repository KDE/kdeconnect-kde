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
#include <QDebug>

#include "kdeconnectconfig.h"

struct KdeConnectPluginConfigPrivate
{
    QDir m_configDir;
    QSettings* m_config;
    QDBusMessage m_signal;
};

KdeConnectPluginConfig::KdeConnectPluginConfig()
    : d(new KdeConnectPluginConfigPrivate())
{

}

KdeConnectPluginConfig::KdeConnectPluginConfig(const QString& deviceId, const QString& pluginName)
    : d(new KdeConnectPluginConfigPrivate())
{
    d->m_configDir = KdeConnectConfig::instance()->pluginConfigDir(deviceId, pluginName);
    QDir().mkpath(d->m_configDir.path());

    d->m_config = new QSettings(d->m_configDir.absoluteFilePath(QStringLiteral("config")), QSettings::IniFormat);

    d->m_signal = QDBusMessage::createSignal("/kdeconnect/" + deviceId + "/" + pluginName, QStringLiteral("org.kde.kdeconnect.config"), QStringLiteral("configChanged"));
    QDBusConnection::sessionBus().connect(QLatin1String(""), "/kdeconnect/" + deviceId + "/" + pluginName, QStringLiteral("org.kde.kdeconnect.config"), QStringLiteral("configChanged"), this, SLOT(slotConfigChanged()));

}

KdeConnectPluginConfig::~KdeConnectPluginConfig()
{
    delete d->m_config;
}

QString KdeConnectPluginConfig::getString(const QString& key, const QString& defaultValue)
{
    if (!d->m_config) {
        loadConfig();
    }

    d->m_config->sync();
    return d->m_config->value(key, defaultValue).toString();
}

bool KdeConnectPluginConfig::getBool(const QString& key, const bool defaultValue)
{
    if (!d->m_config) {
        loadConfig();
    }

    d->m_config->sync();
    return d->m_config->value(key, defaultValue).toBool();
}

int KdeConnectPluginConfig::getInt(const QString& key, const int defaultValue)
{
    if (!d->m_config) {
        loadConfig();
    }

    d->m_config->sync();
    return d->m_config->value(key, defaultValue).toInt();
}

QByteArray KdeConnectPluginConfig::getByteArray(const QString& key, const QByteArray defaultValue)
{
    if (!d->m_config) {
        loadConfig();
    }

    d->m_config->sync();
    return d->m_config->value(key, defaultValue).toByteArray();
}

QVariantList KdeConnectPluginConfig::getList(const QString& key,
                                             const QVariantList& defaultValue)
{
    QVariantList list;
    d->m_config->sync();  // note: need sync() to get recent changes signalled from other process
    int size = d->m_config->beginReadArray(key);
    if (size < 1) {
        d->m_config->endArray();
        return defaultValue;
    }
    for (int i = 0; i < size; ++i) {
        d->m_config->setArrayIndex(i);
        list << d->m_config->value(QStringLiteral("value"));
    }
    d->m_config->endArray();
    return list;
}

void KdeConnectPluginConfig::set(const QString& key, const QVariant& value)
{
    d->m_config->setValue(key, value);
    d->m_config->sync();
    QDBusConnection::sessionBus().send(d->m_signal);
}

void KdeConnectPluginConfig::setList(const QString& key, const QVariantList& list)
{
    d->m_config->beginWriteArray(key);
    for (int i = 0; i < list.size(); ++i) {
        d->m_config->setArrayIndex(i);
        d->m_config->setValue(QStringLiteral("value"), list.at(i));
    }
    d->m_config->endArray();
    d->m_config->sync();
    QDBusConnection::sessionBus().send(d->m_signal);
}

void KdeConnectPluginConfig::slotConfigChanged()
{
    Q_EMIT configChanged();
}

void KdeConnectPluginConfig::setDeviceId(const QString& deviceId)
{
    if (deviceId != m_deviceId) {
        m_deviceId = deviceId;
    }

    if (!m_deviceId.isEmpty() && !m_pluginName.isEmpty()) {
        loadConfig();
    }
}

QString KdeConnectPluginConfig::deviceId()
{
    return m_deviceId;
}

void KdeConnectPluginConfig::setPluginName(const QString& pluginName)
{
    if (pluginName != m_pluginName) {
        m_pluginName = pluginName;
    }

    if (!m_deviceId.isEmpty() && !m_pluginName.isEmpty()) {
        loadConfig();
    }
}

QString KdeConnectPluginConfig::pluginName()
{
    return m_pluginName;
}

void KdeConnectPluginConfig::loadConfig()
{
    d->m_configDir = KdeConnectConfig::instance()->pluginConfigDir(m_deviceId, m_pluginName);
    QDir().mkpath(d->m_configDir.path());

    d->m_config = new QSettings(d->m_configDir.absoluteFilePath(QStringLiteral("config")), QSettings::IniFormat);

    d->m_signal = QDBusMessage::createSignal("/kdeconnect/" + m_deviceId + "/" + m_pluginName, QStringLiteral("org.kde.kdeconnect.config"), QStringLiteral("configChanged"));
    QDBusConnection::sessionBus().connect(QLatin1String(""), "/kdeconnect/" + m_deviceId + "/" + m_pluginName, QStringLiteral("org.kde.kdeconnect.config"), QStringLiteral("configChanged"), this, SLOT(slotConfigChanged()));
    Q_EMIT configChanged();
}
