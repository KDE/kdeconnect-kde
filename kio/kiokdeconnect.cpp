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
    : WorkerBase("kdeconnect", pool, app)
    , m_dbusInterface(new DaemonDbusInterface(this))
{
}

KIO::WorkerResult KioKdeconnect::listAllDevices()
{
    infoMessage(i18n("Listing devices..."));

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
    infoMessage(i18n("Accessing device..."));

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

    QVariantMap urls = urlreply.value();

    for (QVariantMap::iterator it = urls.begin(); it != urls.end(); ++it) {
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
        entry.fastInsert(KIO::UDSEntry::UDS_MIME_TYPE, QLatin1String(""));
        entry.fastInsert(KIO::UDSEntry::UDS_URL, QUrl::fromLocalFile(path).toString());
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

    QString currentDevice = url.host();

    if (currentDevice.isEmpty()) {
        return listAllDevices();
    } else {
        return listDevice(currentDevice);
    }
}

KIO::WorkerResult KioKdeconnect::stat(const QUrl &url)
{
    qCDebug(KDECONNECT_KIO) << "Stat: " << url;

    KIO::UDSEntry entry;
    entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, QT_STAT_DIR);

    QString currentDevice = url.host();
    if (!currentDevice.isEmpty()) {
        SftpDbusInterface interface(currentDevice);

        if (interface.isValid()) {
            const QDBusReply<QString> mountPoint = interface.mountPoint();

            if (!mountPoint.isValid()) {
                return KIO::WorkerResult::fail(KIO::ERR_WORKER_DEFINED, i18n("Failed to get mount point: %1", mountPoint.error().message()));
            } else {
                entry.fastInsert(KIO::UDSEntry::UDS_LOCAL_PATH, interface.mountPoint());
            }

            if (!interface.isMounted()) {
                interface.mount();
            }
        }
    }

    statEntry(entry);

    return KIO::WorkerResult::pass();
}

KIO::WorkerResult KioKdeconnect::get(const QUrl &url)
{
    qCDebug(KDECONNECT_KIO) << "Get: " << url;
    mimeType(QLatin1String(""));
    return KIO::WorkerResult::pass();
}

#include "kiokdeconnect.moc"
#include "moc_kiokdeconnect.cpp"
