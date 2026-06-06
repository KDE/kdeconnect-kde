/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "kiokdeconnect.h"

#include <QDBusMetaType>
#include <QThread>

#include <KLocalizedString>

#include <QDebug>
#include <QtPlugin>

#include "kdeconnectkio_debug.h"

class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.kdeconnect" FILE "kdeconnect.json")
};

class KdeConnectUrl : public QUrl
{
public:
    KdeConnectUrl(const QUrl &url)
        : QUrl(url)
    {
    }

    QString deviceId() const
    {
        return host();
    }

    QString storageDirectory() const
    {
        QString storageDir = path().section(QLatin1Char('/'), 0, 1).mid(1);
        if (storageDir != QLatin1Char('/')) {
            return storageDir;
        } else {
            return QString();
        }
    }

    QString relativePath() const
    {
        QString relativePath = path();
        const int secondSlashIdx = relativePath.indexOf(QLatin1Char('/'), 1);
        if (secondSlashIdx != -1) {
            return relativePath.mid(secondSlashIdx + 1);
        } else {
            return QString();
        }
    }
};

extern "C" int Q_DECL_EXPORT kdemain(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kio_kdeconnect"));

    if (argc != 4) {
        fprintf(stderr, "Usage: kio_kdeconnect protocol pool app\n");
        exit(-1);
    }

    KioKdeconnect worker(argv[2], argv[3]);
    worker.dispatchLoop();
    return 0;
}

// Some useful error mapping
KIO::Error toKioError(const QDBusError::ErrorType type)
{
    switch (type) {
    case QDBusError::NoError:
        return KIO::Error(KJob::NoError);
    case QDBusError::NoMemory:
        return KIO::ERR_OUT_OF_MEMORY;
    case QDBusError::Timeout:
        return KIO::ERR_SERVER_TIMEOUT;
    case QDBusError::TimedOut:
        return KIO::ERR_SERVER_TIMEOUT;
    default:
        return KIO::ERR_WORKER_DEFINED;
    };
};

template<typename T>
KIO::WorkerResult handleDBusError(QDBusReply<T> &reply)
{
    if (!reply.isValid()) {
        qCDebug(KDECONNECT_KIO) << "Error in DBus request:" << reply.error();
        return KIO::WorkerResult::fail(toKioError(reply.error().type()), reply.error().message());
    }
    return KIO::WorkerResult::pass();
}

KioKdeconnect::KioKdeconnect(const QByteArray &pool, const QByteArray &app)
    : ForwardingWorkerBase("kdeconnect", pool, app)
    , m_dbusInterface(new DaemonDbusInterface(this))
{
}

bool KioKdeconnect::rewriteUrl(const QUrl &url, QUrl &newUrl)
{
    const KdeConnectUrl kurl(url);
    if (kurl.deviceId().isEmpty()) {
        return false;
    }

    SftpDbusInterface interface(kurl.deviceId());

    const auto directoriesReply = interface.getDirectories();
    if (directoriesReply.isValid()) {
        qCDebug(KDECONNECT_KIO) << "Error in DBus request:" << directoriesReply.error();
        return false;
    }

    const auto directories = directoriesReply.value();
    const QString storageDirectory = kurl.storageDirectory();

    QString dirPath;
    for (auto it = directories.begin(); it != directories.end(); ++it) {
        // TODO Are storage names unique?
        if (it.value().toString() == storageDirectory) {
            dirPath = it.key();
            break;
        }
    }

    if (dirPath.isEmpty()) {
        return false;
    }

    newUrl = QUrl::fromLocalFile(dirPath + QLatin1Char('/')).resolved(QUrl(kurl.relativePath()));
    return true;
}

void KioKdeconnect::adjustUDSEntry(KIO::UDSEntry &entry, UDSEntryCreationMode creationMode) const
{
    const auto mode = entry.numberValue(KIO::UDSEntry::UDS_FILE_TYPE);
    if ((mode & QT_STAT_MASK) == QT_STAT_DIR) {
        static const QHash<QString, QString> s_folderIcons = {
            {QStringLiteral("Audiobooks"), QStringLiteral("folder-book")},
            {QStringLiteral("DCIM"), QStringLiteral("camera-photo")},
            {QStringLiteral("Documents"), QStringLiteral("folder-documents")},
            {QStringLiteral("Download"), QStringLiteral("folder-downloads")},
            {QStringLiteral("Movies"), QStringLiteral("folder-videos")},
            {QStringLiteral("Music"), QStringLiteral("folder-music")},
            {QStringLiteral("Pictures"), QStringLiteral("folder-pictures")},
        };

        KdeConnectUrl kurl(requestedUrl());

        QString path;
        if (creationMode == UDSEntryCreationInStat) {
            // URL points to the file itself.
            KdeConnectUrl kurl(requestedUrl());
            path = kurl.relativePath();
        } else {
            // Can we assume relativePath() ends with a trailing slash?
            path = kurl.relativePath() + entry.stringValue(KIO::UDSEntry::UDS_NAME);
        }

        const QString iconName = s_folderIcons.value(path);
        if (!iconName.isEmpty()) {
            entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, iconName);
        }
    }
}

KIO::WorkerResult KioKdeconnect::listAllDevices()
{
    infoMessage(i18n("Listing devices…"));

    // TODO: Change to all devices and show different icons for connected and disconnected?
    const QStringList devices = m_dbusInterface->devices(true, true);

    for (const QString &deviceId : devices) {
        DeviceDbusInterface interface(deviceId);

        if (!interface.hasPlugin(QStringLiteral("kdeconnect_sftp")))
            continue;

        const QString path = QStringLiteral("kdeconnect://").append(deviceId).append(QStringLiteral("/"));
        const QString name = interface.name();
        const QString icon = QStringLiteral("kdeconnect");

        KIO::UDSEntry entry;
        entry.reserve(6);
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, name);
        entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, icon);
        entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, QT_STAT_DIR);
        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS,
                         QFileDevice::ReadOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther
                             | QFileDevice::ExeOther);
        entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QLatin1String(""));
        entry.fastInsert(KIO::UDSEntry::UDS_URL, path);
        listEntry(entry);
    }

    // We also need a non-null and writable UDSentry for "."
    KIO::UDSEntry entry;
    entry.reserve(4);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, QT_STAT_DIR);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS,
                     QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup | QFileDevice::WriteGroup
                         | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther);
    listEntry(entry);

    infoMessage(QLatin1String(""));
    return KIO::WorkerResult::pass();
}

KIO::WorkerResult KioKdeconnect::listDevice(const QString &device)
{
    infoMessage(i18n("Accessing device…"));

    qCDebug(KDECONNECT_KIO) << "ListDevice" << device;

    SftpDbusInterface interface(device);

    QDBusReply<bool> mountreply = interface.mountAndWait();

    if (mountreply.error().type() == QDBusError::UnknownObject) {
        DaemonDbusInterface daemon;

        auto devsRepl = daemon.devices(false, false);
        devsRepl.waitForFinished();

        if (!devsRepl.value().contains(device)) {
            return KIO::WorkerResult::fail(KIO::ERR_WORKER_DEFINED, i18n("No such device: %0").arg(device));
        }

        DeviceDbusInterface dev(device);

        if (!dev.isPaired()) {
            return KIO::WorkerResult::fail(KIO::ERR_WORKER_DEFINED, i18n("%0 is not paired").arg(dev.name()));
        }

        if (!dev.isReachable()) {
            return KIO::WorkerResult::fail(KIO::ERR_WORKER_DEFINED, i18n("%0 is not connected").arg(dev.name()));
        }

        if (!dev.hasPlugin(QStringLiteral("kdeconnect_sftp"))) {
            return KIO::WorkerResult::fail(KIO::ERR_WORKER_DEFINED, i18n("%0 has no Remote Filesystem plugin").arg(dev.name()));
        }
    }

    if (auto result = handleDBusError(mountreply); !result.success()) {
        return result;
    }

    if (!mountreply.value()) {
        return KIO::WorkerResult::fail(KIO::ERR_WORKER_DEFINED, interface.getMountError());
    }

    QDBusReply<QVariantMap> urlreply = interface.getDirectories();

    if (auto result = handleDBusError(urlreply); !result.success()) {
        return result;
    }

    const QVariantMap urls = urlreply.value();

    // If there is only a single storage location, redirect into it.
    if (urls.size() == 1) {
        const QUrl storageUrl(QStringLiteral("kdeconnect://%1/%2/").arg(device, urls.begin().value().toString()));
        qCDebug(KDECONNECT_KIO) << "Single storage found on" << device << "redirecting to" << storageUrl;
        redirection(storageUrl);
        return KIO::WorkerResult::pass();
    }

    for (auto it = urls.begin(); it != urls.end(); ++it) {
        const QString path = it.key();
        const QString name = it.value().toString();
        const QString icon = QStringLiteral("folder");

        KIO::UDSEntry entry;
        entry.reserve(6);
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, name);
        entry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, icon);
        entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, QT_STAT_DIR);
        entry.fastInsert(KIO::UDSEntry::UDS_ACCESS,
                         QFileDevice::ReadOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup | QFileDevice::ExeGroup | QFileDevice::ReadOther
                             | QFileDevice::ExeOther);
        listEntry(entry);
    }

    // We also need a non-null and writable UDSentry for "."
    KIO::UDSEntry entry;
    entry.reserve(4);
    entry.fastInsert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, QT_STAT_DIR);
    entry.fastInsert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.fastInsert(KIO::UDSEntry::UDS_ACCESS,
                     QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner | QFileDevice::ReadGroup | QFileDevice::WriteGroup
                         | QFileDevice::ExeGroup | QFileDevice::ReadOther | QFileDevice::ExeOther);

    listEntry(entry);

    infoMessage(QLatin1String(""));
    return KIO::WorkerResult::pass();
}

KIO::WorkerResult KioKdeconnect::listDir(const QUrl &url)
{
    qCDebug(KDECONNECT_KIO) << "Listing..." << url;

    if (!m_dbusInterface->isValid()) {
        return KIO::WorkerResult::fail(KIO::Error::ERR_WORKER_DEFINED, i18n("Could not contact background service."));
    }

    const KdeConnectUrl kurl(url);

    if (kurl.deviceId().isEmpty()) {
        return listAllDevices();
    }

    if (kurl.storageDirectory().isEmpty()) {
        return listDevice(kurl.deviceId());
    }

    return KIO::ForwardingWorkerBase::listDir(url);
}

KIO::WorkerResult KioKdeconnect::stat(const QUrl &url)
{
    qCDebug(KDECONNECT_KIO) << "Stat:" << url;

    const KdeConnectUrl kurl(url);
    const QString currentDevice = kurl.deviceId();

    KIO::UDSEntry dirEntry;
    dirEntry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, QT_STAT_DIR);

    if (currentDevice.isEmpty()) {
        dirEntry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, i18n("KDE Connect"));
        dirEntry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, QStringLiteral("kdeconnect"));
        statEntry(dirEntry);
        return KIO::WorkerResult::pass();
    }

    SftpDbusInterface interface(currentDevice);

    const QDBusReply<QString> mountPointReply = interface.mountPoint();
    if (!mountPointReply.isValid()) {
        return KIO::WorkerResult::fail(KIO::ERR_WORKER_DEFINED, i18n("Failed to get mount point: %1", mountPointReply.error().message()));
    }

    if (!interface.isMounted()) {
        interface.mount();
    }

    if (kurl.storageDirectory().isEmpty()) {
        DeviceDbusInterface interface(kurl.deviceId());
        if (const QString deviceName = interface.name(); !deviceName.isEmpty()) {
            dirEntry.fastInsert(KIO::UDSEntry::UDS_DISPLAY_NAME, deviceName);
        }
        dirEntry.fastInsert(KIO::UDSEntry::UDS_ICON_NAME, QStringLiteral("kdeconnect"));
        statEntry(dirEntry);
        return KIO::WorkerResult::pass();
    }

    if (kurl.relativePath().isEmpty()) {
        statEntry(dirEntry);
        return KIO::WorkerResult::pass();
    }

    return KIO::ForwardingWorkerBase::stat(url);
}

KIO::WorkerResult KioKdeconnect::get(const QUrl &url)
{
    qCDebug(KDECONNECT_KIO) << "Get:" << url;

    const KdeConnectUrl kurl(url);
    if (!kurl.deviceId().isEmpty() && !kurl.storageDirectory().isEmpty()) {
        return KIO::ForwardingWorkerBase::get(url);
    }

    mimeType(QLatin1String(""));
    return KIO::WorkerResult::pass();
}

#include "kiokdeconnect.moc"
#include "moc_kiokdeconnect.cpp"
