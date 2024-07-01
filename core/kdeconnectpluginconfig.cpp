/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "kdeconnectpluginconfig.h"

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDebug>
#include <QDir>
#include <QSettings>

#include "dbushelper.h"
#include "kdeconnectconfig.h"

struct KdeConnectPluginConfigPrivate {
    QDir m_configDir;
    QSettings *m_config;
    QDBusMessage m_signal;
};

KdeConnectPluginConfig::KdeConnectPluginConfig(QObject *parent)
    : QObject(parent)
    , d(new KdeConnectPluginConfigPrivate())
{
}

KdeConnectPluginConfig::KdeConnectPluginConfig(const QString &deviceId, const QString &pluginName, QObject *parent)
    : QObject(parent)
    , d(new KdeConnectPluginConfigPrivate())
    , m_deviceId(deviceId)
    , m_pluginName(pluginName)
{
    d->m_configDir = KdeConnectConfig::instance().pluginConfigDir(deviceId, pluginName);
    QDir().mkpath(d->m_configDir.path());

    d->m_config = new QSettings(d->m_configDir.absoluteFilePath(QStringLiteral("config")), QSettings::IniFormat);

    d->m_signal = QDBusMessage::createSignal(QStringLiteral("/kdeconnect/") + deviceId + QStringLiteral("/") + pluginName,
                                             QStringLiteral("org.kde.kdeconnect.config"),
                                             QStringLiteral("configChanged"));
    QDBusConnection::sessionBus().connect(QLatin1String(""),
                                          QStringLiteral("/kdeconnect/") + deviceId + QStringLiteral("/") + pluginName,
                                          QStringLiteral("org.kde.kdeconnect.config"),
                                          QStringLiteral("configChanged"),
                                          this,
                                          SLOT(slotConfigChanged()));
}

KdeConnectPluginConfig::~KdeConnectPluginConfig()
{
    delete d->m_config;
}

QString KdeConnectPluginConfig::getString(const QString &key, const QString &defaultValue)
{
    if (!d->m_config) {
        loadConfig();
    }

    d->m_config->sync();
    return d->m_config->value(key, defaultValue).toString();
}

bool KdeConnectPluginConfig::getBool(const QString &key, const bool defaultValue)
{
    if (!d->m_config) {
        loadConfig();
    }

    d->m_config->sync();
    return d->m_config->value(key, defaultValue).toBool();
}

int KdeConnectPluginConfig::getInt(const QString &key, const int defaultValue)
{
    if (!d->m_config) {
        loadConfig();
    }

    d->m_config->sync();
    return d->m_config->value(key, defaultValue).toInt();
}

QByteArray KdeConnectPluginConfig::getByteArray(const QString &key, const QByteArray defaultValue)
{
    if (!d->m_config) {
        loadConfig();
    }

    d->m_config->sync();
    return d->m_config->value(key, defaultValue).toByteArray();
}

QVariantList KdeConnectPluginConfig::getList(const QString &key, const QVariantList &defaultValue)
{
    QVariantList list;
    d->m_config->sync(); // note: need sync() to get recent changes signalled from other process
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

void KdeConnectPluginConfig::set(const QString &key, const QVariant &value)
{
    d->m_config->setValue(key, value);
    d->m_config->sync();
    QDBusConnection::sessionBus().send(d->m_signal);
}

void KdeConnectPluginConfig::setList(const QString &key, const QVariantList &list)
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

void KdeConnectPluginConfig::setDeviceId(const QString &deviceId)
{
    if (deviceId != m_deviceId) {
        m_deviceId = deviceId;
    }

    if (!m_deviceId.isEmpty() && !m_pluginName.isEmpty()) {
        loadConfig();
    }

    Q_EMIT deviceIdChanged(deviceId);
}

QString KdeConnectPluginConfig::deviceId()
{
    return m_deviceId;
}

void KdeConnectPluginConfig::setPluginName(const QString &pluginName)
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
    d->m_configDir = KdeConnectConfig::instance().pluginConfigDir(m_deviceId, m_pluginName);
    QDir().mkpath(d->m_configDir.path());

    d->m_config = new QSettings(d->m_configDir.absoluteFilePath(QStringLiteral("config")), QSettings::IniFormat);

    d->m_signal = QDBusMessage::createSignal(QStringLiteral("/kdeconnect/") + m_deviceId + QStringLiteral("/") + m_pluginName,
                                             QStringLiteral("org.kde.kdeconnect.config"),
                                             QStringLiteral("configChanged"));
    QDBusConnection::sessionBus().connect(QLatin1String(""),
                                          QStringLiteral("/kdeconnect/") + m_deviceId + QStringLiteral("/") + m_pluginName,
                                          QStringLiteral("org.kde.kdeconnect.config"),
                                          QStringLiteral("configChanged"),
                                          this,
                                          SLOT(slotConfigChanged()));
    Q_EMIT configChanged();
}

#include "moc_kdeconnectpluginconfig.cpp"
