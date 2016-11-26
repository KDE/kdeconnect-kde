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

#include <KLocalizedString>

#include <QFile>
#include <QDebug>
#include <QFileInfo>
#include <QUuid>
#include <QDir>
#include <QStandardPaths>
#include <QCoreApplication>
#include <QHostInfo>
#include <QSettings>
#include <QSslCertificate>
#include <QtCrypto>
#include <QSslCertificate>

#include "core_debug.h"
#include "dbushelper.h"
#include "daemon.h"

struct KdeConnectConfigPrivate {

    // The Initializer object sets things up, and also does cleanup when it goes out of scope
    // Note it's not being used anywhere. That's intended
    QCA::Initializer mQcaInitializer;

    QCA::PrivateKey privateKey;
    QSslCertificate certificate; // Use QSslCertificate instead of QCA::Certificate due to compatibility with QSslSocket

    QSettings* config;
    QSettings* trusted_devices;

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
        qCritical() << "Could not find support for RSA in your QCA installation";
        Daemon::instance()->reportError(
                             i18n("KDE Connect failed to start"),
                             i18n("Could not find support for RSA in your QCA installation. If your "
                                  "distribution provides separate packages for QCA-ossl and QCA-gnupg, "
                                  "make sure you have them installed and try again."));
        return;
    }

    //Make sure base directory exists
    QDir().mkpath(baseConfigDir().path());

    //.config/kdeconnect/config
    d->config = new QSettings(baseConfigDir().absoluteFilePath(QStringLiteral("config")), QSettings::IniFormat);
    d->trusted_devices = new QSettings(baseConfigDir().absoluteFilePath(QStringLiteral("trusted_devices")), QSettings::IniFormat);

    //Register my own id if not there yet
    if (!d->config->contains(QStringLiteral("id"))) {
        QString uuid = QUuid::createUuid().toString();
        DbusHelper::filterNonExportableCharacters(uuid);
        d->config->setValue(QStringLiteral("id"), uuid);
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
            Daemon::instance()->reportError(QStringLiteral("KDE Connect"), i18n("Could not store private key file: %1", keyPath));
        } else {
            privKey.setPermissions(strict);
            privKey.write(d->privateKey.toPEM().toLatin1());
        }
    }

    QString certPath = certificatePath();
    QFile cert(certPath);
    if (cert.exists() && cert.open(QIODevice::ReadOnly)) {

        d->certificate = QSslCertificate::fromPath(certPath).at(0);

    } else {

        // FIXME: We only use QCA here to generate the cert and key, would be nice to get rid of it completely.
        // The same thing we are doing with QCA could be done invoking openssl (altought it's potentially less portable):
        // openssl req -new -x509 -sha256 -newkey rsa:2048 -nodes -keyout privateKey.pem -days 3650 -out certificate.pem -subj "/O=KDE/OU=KDE Connect/CN=_e6e29ad4_2b31_4b6d_8f7a_9872dbaa9095_"

        QCA::CertificateOptions certificateOptions = QCA::CertificateOptions();
        QDateTime startTime = QDateTime::currentDateTime().addYears(-1);
        QDateTime endTime = startTime.addYears(10);
        QCA::CertificateInfo certificateInfo;
        certificateInfo.insert(QCA::CommonName,deviceId());
        certificateInfo.insert(QCA::Organization,QStringLiteral("KDE"));
        certificateInfo.insert(QCA::OrganizationalUnit,QStringLiteral("Kde connect"));
        certificateOptions.setInfo(certificateInfo);
        certificateOptions.setFormat(QCA::PKCS10);
        certificateOptions.setSerialNumber(QCA::BigInteger(10));
        certificateOptions.setValidityPeriod(startTime, endTime);

        d->certificate = QSslCertificate(QCA::Certificate(certificateOptions, d->privateKey).toPEM().toLatin1());

        if (!cert.open(QIODevice::ReadWrite | QIODevice::Truncate))  {
            Daemon::instance()->reportError(QStringLiteral("KDE Connect"), i18n("Could not store certificate file: %1", certPath));
        } else {
            cert.setPermissions(strict);
            cert.write(d->certificate.toPem());
        }
    }

    //Extra security check
    if (QFile::permissions(keyPath) != strict) {
        qCWarning(KDECONNECT_CORE) << "Warning: KDE Connect private key file has too open permissions " << keyPath;
    }
}

QString KdeConnectConfig::name()
{
    QString defaultName = qgetenv("USER") + '@' + QHostInfo::localHostName();
    QString name = d->config->value(QStringLiteral("name"), defaultName).toString();
    return name;
}

void KdeConnectConfig::setName(const QString& name)
{
    d->config->setValue(QStringLiteral("name"), name);
    d->config->sync();
}

QString KdeConnectConfig::deviceType()
{
    return QStringLiteral("desktop"); // TODO
}

QString KdeConnectConfig::deviceId()
{
    QString id = d->config->value(QStringLiteral("id"), "").toString();
    return id;
}

QString KdeConnectConfig::privateKeyPath()
{
    return baseConfigDir().absoluteFilePath(QStringLiteral("privateKey.pem"));
}

QCA::PrivateKey KdeConnectConfig::privateKey()
{
    return d->privateKey;
}

QCA::PublicKey KdeConnectConfig::publicKey()
{
    return d->privateKey.toPublicKey();
}

QString KdeConnectConfig::certificatePath()
{
    return baseConfigDir().absoluteFilePath(QStringLiteral("certificate.pem"));
}

QSslCertificate KdeConnectConfig::certificate()
{
    return d->certificate;
}

QDir KdeConnectConfig::baseConfigDir()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString kdeconnectConfigPath = QDir(configPath).absoluteFilePath(QStringLiteral("kdeconnect"));
    return QDir(kdeconnectConfigPath);
}

QStringList KdeConnectConfig::trustedDevices()
{
    const QStringList& list = d->trusted_devices->childGroups();
    return list;
}


void KdeConnectConfig::addTrustedDevice(const QString &id, const QString &name, const QString &type)
{
    d->trusted_devices->beginGroup(id);
    d->trusted_devices->setValue(QStringLiteral("name"), name);
    d->trusted_devices->setValue(QStringLiteral("type"), type);
    d->trusted_devices->endGroup();
    d->trusted_devices->sync();

    QDir().mkpath(deviceConfigDir(id).path());
}

KdeConnectConfig::DeviceInfo KdeConnectConfig::getTrustedDevice(const QString &id)
{
    d->trusted_devices->beginGroup(id);

    KdeConnectConfig::DeviceInfo info;
    info.deviceName = d->trusted_devices->value(QStringLiteral("name"), QLatin1String("unnamed")).toString();
    info.deviceType = d->trusted_devices->value(QStringLiteral("type"), QLatin1String("unknown")).toString();

    d->trusted_devices->endGroup();
    return info;
}

void KdeConnectConfig::removeTrustedDevice(const QString &deviceId)
{
    d->trusted_devices->remove(deviceId);
    d->trusted_devices->sync();
    //We do not remove the config files.
}

// Utility functions to set and get a value
void KdeConnectConfig::setDeviceProperty(const QString& deviceId, const QString& key, const QString& value)
{
    d->trusted_devices->beginGroup(deviceId);
    d->trusted_devices->setValue(key, value);
    d->trusted_devices->endGroup();
    d->trusted_devices->sync();
}

QString KdeConnectConfig::getDeviceProperty(const QString& deviceId, const QString& key, const QString& defaultValue)
{
    QString value;
    d->trusted_devices->beginGroup(deviceId);
    value = d->trusted_devices->value(key, defaultValue).toString();
    d->trusted_devices->endGroup();
    return value;
}


QDir KdeConnectConfig::deviceConfigDir(const QString &deviceId)
{
    QString deviceConfigPath = baseConfigDir().absoluteFilePath(deviceId);
    return QDir(deviceConfigPath);
}

QDir KdeConnectConfig::pluginConfigDir(const QString &deviceId, const QString &pluginName)
{
    QString deviceConfigPath = baseConfigDir().absoluteFilePath(deviceId);
    QString pluginConfigDir = QDir(deviceConfigPath).absoluteFilePath(pluginName);
    return QDir(pluginConfigDir);
}


