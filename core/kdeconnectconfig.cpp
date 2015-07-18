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
#include <QSettings>
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
    d->config = new QSettings(baseConfigDir().absoluteFilePath("config"), QSettings::IniFormat);

    //Register my own id if not there yet
    d->config->beginGroup("myself");
    if (!d->config->contains("id")) {
        QString uuid = QUuid::createUuid().toString();
        DbusHelper::filterNonExportableCharacters(uuid);
        d->config->setValue("id", uuid);
        d->config->sync();
        qCDebug(KDECONNECT_CORE) << "My id:" << uuid;
    }
    d->config->endGroup();

    const QFile::Permissions strict = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser;

    QString keyPath = privateKeyPath();
    QFile privKey(keyPath);
    if (privKey.exists() && privKey.open(QIODevice::ReadOnly)) {

        d->privateKey = QCA::PrivateKey::fromPEM(privKey.readAll());

    } else {

        d->privateKey = QCA::KeyGenerator().createRSA(2048);

        if (!privKey.open(QIODevice::ReadWrite | QIODevice::Truncate))  {
            Daemon::instance()->reportError(QLatin1String("KDE Connect"), i18n("Could not store private key file: %1", keyPath));
        } else {
            privKey.setPermissions(strict);
            privKey.write(d->privateKey.toPEM().toLatin1());
        }
    }

    QString certPath = certificatePath();
    QFile cert(certPath);
    if (cert.exists() && cert.open(QIODevice::ReadOnly)) {

        d->certificate = QSslCertificate::fromPath(certPath).first();

    } else {

        QCA::CertificateOptions certificateOptions = QCA::CertificateOptions();
        // TODO : Set serial number for certificate. Time millis or any constant number?
        QCA::BigInteger bigInteger(10);
        QDateTime startTime = QDateTime::currentDateTime();
        QDateTime endTime = startTime.addYears(10);
        QCA::CertificateInfo certificateInfo;
        certificateInfo.insert(QCA::CommonName,d->config->value("id", "unknown id").toString());
        certificateInfo.insert(QCA::Organization,"KDE");
        certificateInfo.insert(QCA::OrganizationalUnit,"Kde connect");
        certificateOptions.setFormat(QCA::PKCS10);

        certificateOptions.setSerialNumber(bigInteger);
        certificateOptions.setInfo(certificateInfo);
        certificateOptions.setValidityPeriod(startTime, endTime);
        certificateOptions.setFormat(QCA::PKCS10);

//        d->certificate = QCA::Certificate(certificateOptions, d->privateKey);
        d->certificate = QSslCertificate(QCA::Certificate(certificateOptions, d->privateKey).toPEM().toLatin1());

        if (!cert.open(QIODevice::ReadWrite | QIODevice::Truncate))  {
            Daemon::instance()->reportError(QLatin1String("KDE Connect"), i18n("Could not store certificate file: %1", certPath));
        } else {
            cert.setPermissions(strict);
            cert.write(d->certificate.toPem());
        }
    }

    //Extra security check
    if (QFile::permissions(keyPath) != strict) {
        qCDebug(KDECONNECT_CORE) << "Warning: KDE Connect private key file has too open permissions " << keyPath;
    }
}

QString KdeConnectConfig::name()
{
    QString defaultName = qgetenv("USER") + "@" + QHostInfo::localHostName();
    d->config->beginGroup("myself");
    QString name = d->config->value("name", defaultName).toString();
    d->config->endGroup();
    return name;
}

void KdeConnectConfig::setName(QString name)
{
    d->config->beginGroup("myself");
    d->config->setValue("name", name);
    d->config->endGroup();
    d->config->sync();
}

QString KdeConnectConfig::deviceType()
{
    return "desktop"; // TODO
}

QString KdeConnectConfig::deviceId()
{
    d->config->beginGroup("myself");
    QString id = d->config->value("id", "").toString();
    d->config->endGroup();
    return id;
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

QString KdeConnectConfig::certificatePath()
{
    return baseConfigDir().absoluteFilePath("certificate.pem");
}

QSslCertificate KdeConnectConfig::certificate()
{
    return d->certificate;
}

QDir KdeConnectConfig::baseConfigDir()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    QString kdeconnectConfigPath = QDir(configPath).absoluteFilePath("kdeconnect");
    return QDir(kdeconnectConfigPath);
}

QStringList KdeConnectConfig::trustedDevices()
{
    d->config->beginGroup("trustedDevices");
    const QStringList& list = d->config->childGroups();
    d->config->endGroup();
    return list;
}

void KdeConnectConfig::addTrustedDevice(QString id, QString name, QString type, QString publicKey, QString certificate)
{
    d->config->beginGroup("trustedDevices");
    d->config->beginGroup(id);
    d->config->setValue("name", name);
    d->config->setValue("type", type);
    d->config->setValue("publicKey", publicKey);
    d->config->setValue("certificate", certificate);
    d->config->endGroup();
    d->config->endGroup();
    d->config->sync();

    QDir().mkpath(deviceConfigDir(id).path());
}

KdeConnectConfig::DeviceInfo KdeConnectConfig::getTrustedDevice(QString id)
{
    d->config->beginGroup("trustedDevices");
    d->config->beginGroup(id);

    KdeConnectConfig::DeviceInfo info;
    info.deviceName = d->config->value("name", QLatin1String("unnamed")).toString();
    info.deviceType = d->config->value("type", QLatin1String("unknown")).toString();
    info.publicKey = d->config->value("publicKey", QString()).toString();
    info.certificate = d->config->value("certificate", QString()).toString();

    d->config->endGroup();
    d->config->endGroup();
    return info;
}

void KdeConnectConfig::removeTrustedDevice(QString deviceId)
{
    d->config->beginGroup("trustedDevices");
    d->config->beginGroup(deviceId);
    d->config->remove(QString());
    d->config->endGroup();
    d->config->endGroup();
    d->config->sync();
    //We do not remove the config files.
}

// Utility functions to set and get a value
void KdeConnectConfig::setDeviceProperty(QString deviceId, QString key, QString value)
{
    d->config->beginGroup("trustedDevices");
    d->config->beginGroup(deviceId);
    d->config->setValue(key, value);
    d->config->endGroup();
    d->config->endGroup();
    d->config->sync();
}

QString KdeConnectConfig::getDeviceProperty(QString deviceId, QString key, QString defaultValue)
{
    QString value;
    d->config->beginGroup("trustedDevices");
    d->config->beginGroup(deviceId);
    value = d->config->value(key, defaultValue).toString();
    d->config->endGroup();
    d->config->endGroup();
    return value;
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

