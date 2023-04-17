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
#include <QtCrypto>

#include "core_debug.h"
#include "daemon.h"
#include "dbushelper.h"
#include "deviceinfo.h"
#include "pluginloader.h"

const QFile::Permissions strictPermissions = QFile::ReadOwner | QFile::WriteOwner | QFile::ReadUser | QFile::WriteUser;

struct KdeConnectConfigPrivate {
    // The Initializer object sets things up, and also does cleanup when it goes out of scope
    // Note it's not being used anywhere. That's intended
    QCA::Initializer m_qcaInitializer;

    QCA::PrivateKey m_privateKey;
    QSslCertificate m_certificate; // Use QSslCertificate instead of QCA::Certificate due to compatibility with QSslSocket

    QSettings *m_config;
    QSettings *m_trustedDevices;

#ifdef Q_OS_MAC
    QString m_privateDBusAddress; // Private DBus Address cache
#endif
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
    // qCDebug(KDECONNECT_CORE) << "QCA supported capabilities:" << QCA::supportedFeatures().join(",");
    if (!QCA::isSupported("rsa")) {
        qCritical() << "Could not find support for RSA in your QCA installation";
        Daemon::instance()->reportError(i18n("KDE Connect failed to start"),
                                        i18n("Could not find support for RSA in your QCA installation. If your "
                                             "distribution provides separate packets for QCA-ossl and QCA-gnupg, "
                                             "make sure you have them installed and try again."));
    }

    // Make sure base directory exists
    QDir().mkpath(baseConfigDir().path());

    //.config/kdeconnect/config
    d->m_config = new QSettings(baseConfigDir().absoluteFilePath(QStringLiteral("config")), QSettings::IniFormat);
    d->m_trustedDevices = new QSettings(baseConfigDir().absoluteFilePath(QStringLiteral("trusted_devices")), QSettings::IniFormat);

    loadPrivateKey();
    loadCertificate();

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

void KdeConnectConfig::loadPrivateKey()
{
    QString keyPath = privateKeyPath();
    QFile privKey(keyPath);

    bool needsToGenerateKey = false;
    if (privKey.exists() && privKey.open(QIODevice::ReadOnly)) {
        QCA::ConvertResult result;
        d->m_privateKey = QCA::PrivateKey::fromPEM(QString::fromLatin1(privKey.readAll()), QCA::SecureArray(), &result);
        if (result != QCA::ConvertResult::ConvertGood) {
            qCWarning(KDECONNECT_CORE) << "Private key from" << keyPath << "is not valid";
            needsToGenerateKey = true;
        }
    } else {
        needsToGenerateKey = true;
    }

    if (needsToGenerateKey) {
        generatePrivateKey(keyPath);
    }

    // Extra security check
    if (QFile::permissions(keyPath) != strictPermissions) {
        qCWarning(KDECONNECT_CORE) << "Warning: KDE Connect private key file has too open permissions " << keyPath;
    }
}

void KdeConnectConfig::loadCertificate()
{
    QString certPath = certificatePath();
    QFile cert(certPath);

    bool needsToGenerateCert = false;
    if (cert.exists() && cert.open(QIODevice::ReadOnly)) {
        auto loadedCerts = QSslCertificate::fromPath(certPath);
        if (loadedCerts.empty()) {
            qCWarning(KDECONNECT_CORE) << "Certificate from" << certPath << "is not valid";
            needsToGenerateCert = true;
        } else {
            d->m_certificate = loadedCerts.at(0);
        }
    } else {
        needsToGenerateCert = true;
    }

    if (needsToGenerateCert) {
        generateCertificate(certPath);
    }

    // Extra security check
    if (QFile::permissions(certPath) != strictPermissions) {
        qCWarning(KDECONNECT_CORE) << "Warning: KDE Connect certificate file has too open permissions " << certPath;
    }
}

void KdeConnectConfig::generatePrivateKey(const QString &keyPath)
{
    qCDebug(KDECONNECT_CORE) << "Generating private key";

    bool error = false;

    d->m_privateKey = QCA::KeyGenerator().createRSA(2048);

    QFile privKey(keyPath);
    if (!privKey.open(QIODevice::ReadWrite | QIODevice::Truncate)) {
        error = true;
    } else {
        privKey.setPermissions(strictPermissions);
        int written = privKey.write(d->m_privateKey.toPEM().toLatin1());
        if (written <= 0) {
            error = true;
        }
    }

    if (error) {
        Daemon::instance()->reportError(QStringLiteral("KDE Connect"), i18n("Could not store private key file: %1", keyPath));
    }
}

void KdeConnectConfig::generateCertificate(const QString &certPath)
{
    qCDebug(KDECONNECT_CORE) << "Generating certificate";

    bool error = false;

    QString uuid = QUuid::createUuid().toString();
    DBusHelper::filterNonExportableCharacters(uuid);
    qCDebug(KDECONNECT_CORE) << "My id:" << uuid;

    // FIXME: We only use QCA here to generate the cert and key, would be nice to get rid of it completely.
    // The same thing we are doing with QCA could be done invoking openssl (although it's potentially less portable):
    // openssl req -new -x509 -sha256 -newkey rsa:2048 -nodes -keyout privateKey.pem -days 3650 -out certificate.pem -subj "/O=KDE/OU=KDE
    // Connect/CN=_e6e29ad4_2b31_4b6d_8f7a_9872dbaa9095_"

    QCA::CertificateOptions certificateOptions = QCA::CertificateOptions();
    QDateTime startTime = QDateTime::currentDateTime().addYears(-1);
    QDateTime endTime = startTime.addYears(10);
    QCA::CertificateInfo certificateInfo;
    certificateInfo.insert(QCA::CommonName, uuid);
    certificateInfo.insert(QCA::Organization, QStringLiteral("KDE"));
    certificateInfo.insert(QCA::OrganizationalUnit, QStringLiteral("Kde connect"));
    certificateOptions.setInfo(certificateInfo);
    certificateOptions.setFormat(QCA::PKCS10);
    certificateOptions.setSerialNumber(QCA::BigInteger(10));
    certificateOptions.setValidityPeriod(startTime, endTime);

    d->m_certificate = QSslCertificate(QCA::Certificate(certificateOptions, d->m_privateKey).toPEM().toLatin1());

    QFile cert(certPath);
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

#ifdef Q_OS_MAC
QString KdeConnectConfig::privateDBusAddressPath()
{
    return baseConfigDir().absoluteFilePath(QStringLiteral("private_dbus_address"));
}

QString KdeConnectConfig::privateDBusAddress()
{
    if (d->m_privateDBusAddress.length() != 0)
        return d->m_privateDBusAddress;

    QString dbusAddressPath = privateDBusAddressPath();
    QFile dbusAddressFile(dbusAddressPath);

    if (!dbusAddressFile.open(QFile::ReadOnly | QFile::Text)) {
        qCCritical(KDECONNECT_CORE) << "Private DBus enabled but error read private dbus address conf";
        exit(1);
    }

    QTextStream in(&dbusAddressFile);

    qCDebug(KDECONNECT_CORE) << "Waiting for private dbus";

    int retry = 0;
    QString addr = in.readLine();
    while (addr.length() == 0 && retry < 5) {
        qCDebug(KDECONNECT_CORE) << "Retry reading private DBus address after 3s";
        QThread::sleep(3);
        retry++;
        addr = in.readLine(); // Read until first not empty line
    }

    if (addr.length() == 0) {
        qCCritical(KDECONNECT_CORE) << "Private DBus enabled but read private dbus address failed";
        exit(1);
    }

    qCDebug(KDECONNECT_CORE) << "Private dbus address: " << addr;

    d->m_privateDBusAddress = addr;

    return addr;
}
#endif
