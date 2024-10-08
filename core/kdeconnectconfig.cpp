/**
 * SPDX-FileCopyrightText: 2015 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "kdeconnectconfig.h"

#include <KLocalizedString>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHostInfo>
#include <QSettings>
#include <QSslCertificate>
#include <QStandardPaths>
#include <QThread>
#include <QUuid>

#include "core_debug.h"
#include "daemon.h"
#include "dbushelper.h"
#include "deviceinfo.h"
#include "pluginloader.h"
#include "sslhelper.h"

const QFile::Permissions strictPermissions = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser;

struct KdeConnectConfigPrivate {
    QSslKey m_privateKey;
    QSslCertificate m_certificate;

    QSettings *m_config;
    QSettings *m_trustedDevices;
};

static QString getDefaultDeviceName()
{
    return QHostInfo::localHostName();
}

KdeConnectConfig &KdeConnectConfig::instance()
{
    static KdeConnectConfig kcc;
    return kcc;
}

KdeConnectConfig::KdeConnectConfig()
    : d(new KdeConnectConfigPrivate)
{
    // Make sure base directory exists
    QDir().mkpath(baseConfigDir().path());

    //.config/kdeconnect/config
    d->m_config = new QSettings(baseConfigDir().absoluteFilePath(QStringLiteral("config")), QSettings::IniFormat);
    d->m_trustedDevices = new QSettings(baseConfigDir().absoluteFilePath(QStringLiteral("trusted_devices")), QSettings::IniFormat);

    loadOrGeneratePrivateKeyAndCertificate(privateKeyPath(), certificatePath());

    if (name().isEmpty()) {
        setName(getDefaultDeviceName());
    }
}

QString KdeConnectConfig::name()
{
    return d->m_config->value(QStringLiteral("name")).toString();
}

void KdeConnectConfig::setName(const QString &name)
{
    d->m_config->setValue(QStringLiteral("name"), name);
    d->m_config->sync();
}

void KdeConnectConfig::setLinkProviderStatus(const QStringList enabledProviders, const QStringList disabledProviders)
{
    d->m_config->setValue(QStringLiteral("enabled_providers"), enabledProviders);
    d->m_config->setValue(QStringLiteral("disabled_providers"), disabledProviders);
    d->m_config->sync();
}

QMap<QString, QStringList> KdeConnectConfig::linkProviderStatus() const
{
    return {{QStringLiteral("enabled"), d->m_config->value(QStringLiteral("enabled_providers")).toStringList()},
            {QStringLiteral("disabled"), d->m_config->value(QStringLiteral("disabled_providers")).toStringList()}};
}

DeviceType KdeConnectConfig::deviceType()
{
    const QByteArrayList platforms = qgetenv("PLASMA_PLATFORM").split(':');

    if (platforms.contains("phone")) {
        return DeviceType::Phone;
    } else if (platforms.contains("tablet")) {
        return DeviceType::Tablet;
    } else if (platforms.contains("mediacenter")) {
        return DeviceType::Tv;
    }

    // TODO non-Plasma mobile platforms

    return DeviceType::Desktop;
}

QString KdeConnectConfig::deviceId()
{
    return d->m_certificate.subjectInfo(QSslCertificate::CommonName).constFirst();
}

QString KdeConnectConfig::privateKeyPath()
{
    return baseConfigDir().absoluteFilePath(QStringLiteral("privateKey.pem"));
}

QString KdeConnectConfig::certificatePath()
{
    return baseConfigDir().absoluteFilePath(QStringLiteral("certificate.pem"));
}

QSslCertificate KdeConnectConfig::certificate()
{
    return d->m_certificate;
}

QSslKey KdeConnectConfig::privateKey()
{
    return d->m_privateKey;
}

DeviceInfo KdeConnectConfig::deviceInfo()
{
    const auto incoming = PluginLoader::instance()->incomingCapabilities();
    const auto outgoing = PluginLoader::instance()->outgoingCapabilities();
    return DeviceInfo(deviceId(),
                      certificate(),
                      name(),
                      deviceType(),
                      NetworkPacket::s_protocolVersion,
                      QSet(incoming.begin(), incoming.end()),
                      QSet(outgoing.begin(), outgoing.end()));
}

QSsl::KeyAlgorithm KdeConnectConfig::privateKeyAlgorithm()
{
    QString value = d->m_config->value(QStringLiteral("keyAlgorithm"), QStringLiteral("RSA")).toString();
    if (value == QLatin1String("EC")) {
        return QSsl::KeyAlgorithm::Ec;
    } else {
        return QSsl::KeyAlgorithm::Rsa;
    }
}

QDir KdeConnectConfig::baseConfigDir()
{
    QString configPath = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);
    QString kdeconnectConfigPath = QDir(configPath).absoluteFilePath(QStringLiteral("kdeconnect"));
    return QDir(kdeconnectConfigPath);
}

QStringList KdeConnectConfig::trustedDevices()
{
    const QStringList &list = d->m_trustedDevices->childGroups();
    return list;
}

void KdeConnectConfig::addTrustedDevice(const DeviceInfo &deviceInfo)
{
    d->m_trustedDevices->beginGroup(deviceInfo.id);
    d->m_trustedDevices->setValue(QStringLiteral("name"), deviceInfo.name);
    d->m_trustedDevices->setValue(QStringLiteral("type"), deviceInfo.type.toString());
    QString certString = QString::fromLatin1(deviceInfo.certificate.toPem());
    d->m_trustedDevices->setValue(QStringLiteral("certificate"), certString);
    d->m_trustedDevices->endGroup();
    d->m_trustedDevices->sync();

    QDir().mkpath(deviceConfigDir(deviceInfo.id).path());
}

void KdeConnectConfig::updateTrustedDeviceInfo(const DeviceInfo &deviceInfo)
{
    if (!trustedDevices().contains(deviceInfo.id)) {
        // do not store values for untrusted devices (it would make them trusted)
        return;
    }

    d->m_trustedDevices->beginGroup(deviceInfo.id);
    d->m_trustedDevices->setValue(QStringLiteral("name"), deviceInfo.name);
    d->m_trustedDevices->setValue(QStringLiteral("type"), deviceInfo.type.toString());
    d->m_trustedDevices->endGroup();
    d->m_trustedDevices->sync();
}

QSslCertificate KdeConnectConfig::getTrustedDeviceCertificate(const QString &id)
{
    d->m_trustedDevices->beginGroup(id);
    QString certString = d->m_trustedDevices->value(QStringLiteral("certificate"), QString()).toString();
    d->m_trustedDevices->endGroup();
    return QSslCertificate(certString.toLatin1());
}

DeviceInfo KdeConnectConfig::getTrustedDevice(const QString &id)
{
    d->m_trustedDevices->beginGroup(id);

    QString certString = d->m_trustedDevices->value(QStringLiteral("certificate"), QString()).toString();
    QSslCertificate certificate(certString.toLatin1());
    QString name = d->m_trustedDevices->value(QStringLiteral("name"), QLatin1String("unnamed")).toString();
    QString typeString = d->m_trustedDevices->value(QStringLiteral("type"), QLatin1String("unknown")).toString();
    DeviceType type = DeviceType::FromString(typeString);

    d->m_trustedDevices->endGroup();

    return DeviceInfo(id, certificate, name, type);
}

void KdeConnectConfig::removeTrustedDevice(const QString &deviceId)
{
    d->m_trustedDevices->remove(deviceId);
    d->m_trustedDevices->sync();
    // We do not remove the config files.
}

void KdeConnectConfig::removeAllTrustedDevices()
{
    d->m_trustedDevices->clear();
    d->m_trustedDevices->sync();
}

// Utility functions to set and get a value
void KdeConnectConfig::setDeviceProperty(const QString &deviceId, const QString &key, const QString &value)
{
    // do not store values for untrusted devices (it would make them trusted)
    if (!trustedDevices().contains(deviceId))
        return;

    d->m_trustedDevices->beginGroup(deviceId);
    d->m_trustedDevices->setValue(key, value);
    d->m_trustedDevices->endGroup();
    d->m_trustedDevices->sync();
}

QString KdeConnectConfig::getDeviceProperty(const QString &deviceId, const QString &key, const QString &defaultValue)
{
    QString value;
    d->m_trustedDevices->beginGroup(deviceId);
    value = d->m_trustedDevices->value(key, defaultValue).toString();
    d->m_trustedDevices->endGroup();
    return value;
}

void KdeConnectConfig::setCustomDevices(const QStringList &addresses)
{
    d->m_config->setValue(QStringLiteral("customDevices"), addresses);
    d->m_config->sync();
}

QStringList KdeConnectConfig::customDevices() const
{
    return d->m_config->value(QStringLiteral("customDevices")).toStringList();
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

bool KdeConnectConfig::loadPrivateKey(const QString &keyPath)
{
    QFile privKey(keyPath);
    if (privKey.exists() && privKey.open(QIODevice::ReadOnly)) {
        d->m_privateKey = QSslKey(privKey.readAll(), privateKeyAlgorithm());
        if (d->m_privateKey.isNull()) {
            qCWarning(KDECONNECT_CORE) << "Private key from" << keyPath << "is not valid!";
        }
    }
    return d->m_privateKey.isNull();
}

bool KdeConnectConfig::loadCertificate(const QString &certPath)
{
    QFile cert(certPath);
    QDateTime now = QDateTime::currentDateTime();

    if (cert.exists() && cert.open(QIODevice::ReadOnly)) {
        d->m_certificate = QSslCertificate(cert.readAll());
        if (d->m_certificate.isNull()) {
            qCWarning(KDECONNECT_CORE) << "Certificate from" << certPath << "is not valid";
        } else if (d->m_certificate.effectiveDate() >= now) {
            qCWarning(KDECONNECT_CORE) << "Certificate from" << certPath << "not yet effective: " << d->m_certificate.effectiveDate();
            return true;
        } else if (d->m_certificate.expiryDate() <= now) {
            qCWarning(KDECONNECT_CORE) << "Certificate from" << certPath << "expired: " << d->m_certificate.expiryDate();
            return true;
        }
    }
    return d->m_certificate.isNull();
}

void KdeConnectConfig::loadOrGeneratePrivateKeyAndCertificate(const QString &keyPath, const QString &certPath)
{
    bool needsToGenerateKey = loadPrivateKey(keyPath);
    bool needsToGenerateCert = needsToGenerateKey || loadCertificate(certPath);

    if (needsToGenerateKey) {
        generatePrivateKey(keyPath);
    }
    if (needsToGenerateCert) {
        removeAllTrustedDevices();
        generateCertificate(certPath);
    }

    // Extra security check
    if (QFile::permissions(keyPath) != strictPermissions) {
        qCWarning(KDECONNECT_CORE) << "Warning: KDE Connect private key file has too open permissions " << keyPath;
    }
    if (QFile::permissions(certPath) != strictPermissions) {
        qCWarning(KDECONNECT_CORE) << "Warning: KDE Connect certificate file has too open permissions " << certPath;
    }
}

void KdeConnectConfig::generatePrivateKey(const QString &keyPath)
{
    qCDebug(KDECONNECT_CORE) << "Generating private key";

    d->m_privateKey = SslHelper::generateEcPrivateKey();
    if (d->m_privateKey.isNull()) {
        qCritical() << "Could not generate the private key";
        Daemon::instance()->reportError(i18n("KDE Connect failed to start"), i18n("Could not generate the private key."));
    }

    QFile privKey(keyPath);
    bool error = false;
    if (!privKey.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        error = true;
    } else {
        privKey.setPermissions(strictPermissions);
        int written = privKey.write(d->m_privateKey.toPem());
        if (written <= 0) {
            error = true;
        }
    }

    d->m_config->setValue(QStringLiteral("keyAlgorithm"), QStringLiteral("EC"));
    d->m_config->sync();

    if (error) {
        Daemon::instance()->reportError(QStringLiteral("KDE Connect"), i18n("Could not store private key file: %1", keyPath));
    }
}

void KdeConnectConfig::generateCertificate(const QString &certPath)
{
    qCDebug(KDECONNECT_CORE) << "Generating certificate";

    QString uuid = QUuid::createUuid().toString();
    DBusHelper::filterNonExportableCharacters(uuid);
    qCDebug(KDECONNECT_CORE) << "My id:" << uuid;

    d->m_certificate = SslHelper::generateSelfSignedCertificate(d->m_privateKey, uuid);
    if (d->m_certificate.isNull()) {
        qCritical() << "Could not generate a certificate";
        Daemon::instance()->reportError(i18n("KDE Connect failed to start"), i18n("Could not generate the device certificate."));
    }

    QFile cert(certPath);
    bool error = false;
    if (!cert.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        error = true;
    } else {
        cert.setPermissions(strictPermissions);
        int written = cert.write(d->m_certificate.toPem());
        if (written <= 0) {
            error = true;
        }
    }

    if (error) {
        Daemon::instance()->reportError(QStringLiteral("KDE Connect"), i18n("Could not store certificate file: %1", certPath));
    }
}
