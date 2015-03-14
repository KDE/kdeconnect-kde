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

#include "kdeconnectconfig.h"

#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KNotification>
#include <KLocalizedString>

#include <QtCrypto>
#include <QFile>
#include <QDebug>
#include <QFileInfo>
#include <QUuid>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QHostInfo>

#include "core_debug.h"
#include "dbushelper.h"

struct KdeConnectConfigPrivate {

    // The Initializer object sets things up, and also does cleanup when it goes out of scope
    // Note it's not being used anywhere. That's inteneded
    QCA::Initializer mQcaInitializer;

    QCA::PrivateKey privateKey;

    KSharedConfigPtr config;

};

KdeConnectConfig* KdeConnectConfig::instance()
{
    static KdeConnectConfig* kcc = new KdeConnectConfig();
    return kcc;
}

KdeConnectConfig::KdeConnectConfig()
    : d(new KdeConnectConfigPrivate)
{
    //qCDebug(KDECONNECT_CORE) << "QCA supported capabilities:" << QCA::supportedFeatures().join(",");
    if(!QCA::isSupported("rsa")) {
        KNotification::event(KNotification::Error,
                             QLatin1String("KDE Connect failed to start"), //Should not happen, not worth i18n
                             QLatin1String("Could not find support for RSA in your QCA installation. If your"
                                  "distribution provides separate packages for QCA-ossl and QCA-gnupg,"
                                  "make sure you have them installed and try again."));
        return;
    }

    //Make sure base directory exists
    QDir().mkpath(baseConfigDir().path());

    //.config/kdeconnect/config
    d->config = KSharedConfig::openConfig(baseConfigDir().absoluteFilePath("config"), KSharedConfig::SimpleConfig);

    if (!d->config->group("myself").hasKey("id")) {
        QString uuid = QUuid::createUuid().toString();
        DbusHelper::filterNonExportableCharacters(uuid);
        d->config->group("myself").writeEntry("id", uuid);
        d->config->sync();
        qCDebug(KDECONNECT_CORE) << "My id:" << uuid;
    }

    const QFile::Permissions strict = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser;

    QString keyPath = privateKeyPath();
    QFile privKey(keyPath);
    if (privKey.exists() && privKey.open(QIODevice::ReadOnly)) {

        d->privateKey = QCA::PrivateKey::fromPEM(privKey.readAll());

    } else {

        d->privateKey = QCA::KeyGenerator().createRSA(2048);

        if (!privKey.open(QIODevice::ReadWrite | QIODevice::Truncate))  {
            KNotification::event(KNotification::StandardEvent::Error, QLatin1String("KDE Connect"),
                                 i18n("Could not store private key file: %1", keyPath));
        } else {
            privKey.setPermissions(strict);
            privKey.write(d->privateKey.toPEM().toLatin1());
        }
    }

    //Extra security check
    if (QFile::permissions(keyPath) != strict) {
        qCDebug(KDECONNECT_CORE) << "Warning: KDE Connect private key file has too open permissions " << keyPath;
    }
}

QString KdeConnectConfig::name() {
    QString defaultName = qgetenv("USER") + "@" + QHostInfo::localHostName();
    QString name = d->config->group("myself").readEntry("name", defaultName);
    return name;
}

void KdeConnectConfig::setName(QString name)
{
    d->config->group("myself").writeEntry("name", name);
    d->config->sync();
}

QString KdeConnectConfig::deviceType()
{
    return "desktop"; // TODO
}

QString KdeConnectConfig::deviceId() {
    return d->config->group("myself").readEntry("id", "");
}

QString KdeConnectConfig::privateKeyPath()
{
    return baseConfigDir().absoluteFilePath("privateKey.pem");
}

QCA::PrivateKey KdeConnectConfig::privateKey()
{
    return d->privateKey;
}

QCA::PublicKey KdeConnectConfig::publicKey()
{
    return d->privateKey.toPublicKey();
}

QDir KdeConnectConfig::baseConfigDir()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString kdeconnectConfigPath = QDir(configPath).absoluteFilePath("kdeconnect");
    return QDir(kdeconnectConfigPath);
}

QStringList KdeConnectConfig::trustedDevices()
{
    const KConfigGroup& known = d->config->group("trusted_devices");
    const QStringList& list = known.groupList();
    return list;
}

void KdeConnectConfig::addTrustedDevice(QString id, QString name, QString type, QString publicKey)
{
    KConfigGroup device = d->config->group("trusted_devices").group(id);
    device.writeEntry("deviceName", name);
    device.writeEntry("deviceType", type);
    device.writeEntry("publicKey", publicKey);
    d->config->sync();

    QDir().mkpath(deviceConfigDir(id).path());
}

KdeConnectConfig::DeviceInfo KdeConnectConfig::getTrustedDevice(QString id)
{
    KConfigGroup data = d->config->group("trusted_devices").group(id);

    KdeConnectConfig::DeviceInfo info;
    info.deviceName = data.readEntry<QString>("deviceName", QLatin1String("unnamed"));
    info.deviceType = data.readEntry<QString>("deviceType", QLatin1String("unknown"));
    info.publicKey = data.readEntry<QString>("publicKey", QString());

    return info;
}

void KdeConnectConfig::removeTrustedDevice(QString deviceId)
{
    d->config->group("trusted_devices").deleteGroup(deviceId);

    //We do not remove the config files.
}

QDir KdeConnectConfig::deviceConfigDir(QString deviceId)
{
    QString deviceConfigPath = baseConfigDir().absoluteFilePath(deviceId);
    return QDir(deviceConfigPath);
}

QDir KdeConnectConfig::pluginConfigDir(QString deviceId, QString pluginName)
{
    QString deviceConfigPath = baseConfigDir().absoluteFilePath(deviceId);
    QString pluginConfigDir = QDir(deviceConfigPath).absoluteFilePath(pluginName);
    return QDir(pluginConfigDir);
}

