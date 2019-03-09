/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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


#include "kiokdeconnect.h"

#include <QThread>
#include <QDBusMetaType>

#include <KLocalizedString>

#include <QDebug>
#include <QtPlugin>

Q_LOGGING_CATEGORY(KDECONNECT_KIO, "kdeconnect.kio")

class KIOPluginForMetaData : public QObject
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.kde.kio.slave.kdeconnect" FILE "kdeconnect.json")
};

extern "C" int Q_DECL_EXPORT kdemain(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("kio_kdeconnect"));
    
    if (argc != 4) {
        fprintf(stderr, "Usage: kio_kdeconnect protocol pool app\n");
        exit(-1);
    }

    KioKdeconnect slave(argv[2], argv[3]);
    slave.dispatchLoop();
    return 0;
}

//Some useful error mapping
KIO::Error toKioError(const QDBusError::ErrorType type)
{
    switch (type) 
    {
        case QDBusError::NoError:
            return KIO::Error(KJob::NoError);
        case QDBusError::NoMemory:
            return KIO::ERR_OUT_OF_MEMORY;
        case QDBusError::Timeout:          
            return KIO::ERR_SERVER_TIMEOUT;
        case QDBusError::TimedOut:          
            return KIO::ERR_SERVER_TIMEOUT;
        default:
            return KIO::ERR_SLAVE_DEFINED;
    };
};

template <typename T>
bool handleDBusError(QDBusReply<T>& reply, KIO::SlaveBase* slave)
{
    if (!reply.isValid())
    {
        qCDebug(KDECONNECT_KIO) << "Error in DBus request:" << reply.error();
        slave->error(toKioError(reply.error().type()),reply.error().message());
        return true;
    }
    return false;
}

KioKdeconnect::KioKdeconnect(const QByteArray& pool, const QByteArray& app)
    : SlaveBase("kdeconnect", pool, app),
    m_dbusInterface(new DaemonDbusInterface(this))
{

}

void KioKdeconnect::listAllDevices()
{
    infoMessage(i18n("Listing devices..."));

    //TODO: Change to all devices and show different icons for connected and disconnected?
    const QStringList devices = m_dbusInterface->devices(true, true);

    for (const QString& deviceId : devices) {

        DeviceDbusInterface interface(deviceId);

        if (!interface.hasPlugin(QStringLiteral("kdeconnect_sftp"))) continue;

        const QString path = QStringLiteral("kdeconnect://").append(deviceId).append("/");
        const QString name = interface.name();
        const QString icon = QStringLiteral("kdeconnect");

        KIO::UDSEntry entry;
        entry.insert(KIO::UDSEntry::UDS_NAME, name);
        entry.insert(KIO::UDSEntry::UDS_ICON_NAME, icon);
        entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        entry.insert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, QLatin1String(""));
        entry.insert(KIO::UDSEntry::UDS_URL, path);
        listEntry(entry);
    }

    // We also need a non-null and writable UDSentry for "."
    KIO::UDSEntry entry;
    entry.insert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.insert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.insert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);
    listEntry(entry);

    infoMessage(QLatin1String(""));
    finished();
}

void KioKdeconnect::listDevice(const QString& device)
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
            error(KIO::ERR_SLAVE_DEFINED, i18n("No such device: %0").arg(device));
            return;
        }

        DeviceDbusInterface dev(device);

        if (!dev.isTrusted()) {
            error(KIO::ERR_SLAVE_DEFINED, i18n("%0 is not paired").arg(dev.name()));
            return;
        }

        if (!dev.isReachable()) {
            error(KIO::ERR_SLAVE_DEFINED, i18n("%0 is not connected").arg(dev.name()));
            return;
        }

        if (!dev.hasPlugin(QStringLiteral("kdeconnect_sftp"))) {
            error(KIO::ERR_SLAVE_DEFINED, i18n("%0 has no SFTP plugin").arg(dev.name()));
            return;
        }
    }

    if (handleDBusError(mountreply, this)) {
        return;
    }

    if (!mountreply.value()) {
        error(KIO::ERR_SLAVE_DEFINED, interface.getMountError());
        return;
    }

    QDBusReply< QVariantMap > urlreply = interface.getDirectories();

    if (handleDBusError(urlreply, this)) {
        return;
    }

    QVariantMap urls = urlreply.value();

    for (QVariantMap::iterator it = urls.begin(); it != urls.end(); ++it) {

        const QString path = it.key();
        const QString name = it.value().toString();
        const QString icon = QStringLiteral("folder");

        KIO::UDSEntry entry;
        entry.insert(KIO::UDSEntry::UDS_NAME, name);
        entry.insert(KIO::UDSEntry::UDS_ICON_NAME, icon);
        entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
        entry.insert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        entry.insert(KIO::UDSEntry::UDS_MIME_TYPE, QLatin1String(""));
        entry.insert(KIO::UDSEntry::UDS_URL, QUrl::fromLocalFile(path).toString());
        listEntry(entry);
    }

    // We also need a non-null and writable UDSentry for "."
    KIO::UDSEntry entry;
    entry.insert(KIO::UDSEntry::UDS_NAME, QStringLiteral("."));
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);
    entry.insert(KIO::UDSEntry::UDS_SIZE, 0);
    entry.insert(KIO::UDSEntry::UDS_ACCESS, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IXOTH);

    listEntry(entry);

    infoMessage(QLatin1String(""));
    finished();

}



void KioKdeconnect::listDir(const QUrl& url)
{
    qCDebug(KDECONNECT_KIO) << "Listing..." << url;

    if (!m_dbusInterface->isValid()) {
        infoMessage(i18n("Could not contact background service."));
        finished();
        return;
    }

    QString currentDevice = url.host();

    if (currentDevice.isEmpty()) {
        listAllDevices();
    } else {
        listDevice(currentDevice);
    }
}

void KioKdeconnect::stat(const QUrl& url)
{
    qCDebug(KDECONNECT_KIO) << "Stat: " << url;

    KIO::UDSEntry entry;
    entry.insert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFDIR);

    QString currentDevice = url.host();
    if (!currentDevice.isEmpty()) {
        SftpDbusInterface interface(currentDevice);
        entry.insert(KIO::UDSEntry::UDS_LOCAL_PATH, interface.mountPoint());
    }

    statEntry(entry);

    finished();
}

void KioKdeconnect::get(const QUrl& url)
{
    qCDebug(KDECONNECT_KIO) << "Get: " << url;
    mimeType(QLatin1String(""));
    finished();
}

//needed for JSON file embedding
#include "kiokdeconnect.moc"
