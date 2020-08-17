/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "kdeconnectpluginconfig.h"

#include <QDir>
#include <QSettings>
#include <QDBusMessage>

#include "kdeconnectconfig.h"
#include "dbushelper.h"

struct KdeConnectPluginConfigPrivate
{
    QDir m_configDir;
    QSettings* m_config;
    QDBusMessage m_signal;
};

KdeConnectPluginConfig::KdeConnectPluginConfig(const QString& deviceId, const QString& pluginName)
    : d(new KdeConnectPluginConfigPrivate())
{
    d->m_configDir = KdeConnectConfig::instance().pluginConfigDir(deviceId, pluginName);
    QDir().mkpath(d->m_configDir.path());

    d->m_config = new QSettings(d->m_configDir.absoluteFilePath(QStringLiteral("config")), QSettings::IniFormat);

    d->m_signal = QDBusMessage::createSignal(QStringLiteral("/kdeconnect/") + deviceId + QStringLiteral("/") + pluginName, QStringLiteral("org.kde.kdeconnect.config"), QStringLiteral("configChanged"));
    DBusHelper::sessionBus().connect(QLatin1String(""), QStringLiteral("/kdeconnect/") + deviceId + QStringLiteral("/") + pluginName, QStringLiteral("org.kde.kdeconnect.config"), QStringLiteral("configChanged"), this, SLOT(slotConfigChanged()));
}

KdeConnectPluginConfig::~KdeConnectPluginConfig()
{
    delete d->m_config;
}

QVariant KdeConnectPluginConfig::get(const QString& key, const QVariant& defaultValue)
{
    d->m_config->sync();
    return d->m_config->value(key, defaultValue);
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
    DBusHelper::sessionBus().send(d->m_signal);
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
    DBusHelper::sessionBus().send(d->m_signal);
}

void KdeConnectPluginConfig::slotConfigChanged()
{
    Q_EMIT configChanged();
}
