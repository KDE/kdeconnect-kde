/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "sftpplugin.h"

#include <QDBusConnection>
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

#include <KFilePlacesModel>
#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotification>
#include <KNotificationJobUiDelegate>
#include <KPluginFactory>

#include "mounter.h"
#include "plugin_sftp_debug.h"

K_PLUGIN_CLASS_WITH_JSON(SftpPlugin, "kdeconnect_sftp.json")

static const QSet<QString> fields_c = QSet<QString>() << QStringLiteral("ip") << QStringLiteral("port") << QStringLiteral("user") << QStringLiteral("port")
                                                      << QStringLiteral("path");

struct SftpPlugin::Pimpl {
    Pimpl()
        : m_mounter(nullptr)
    {
    }

    // Add KIO entry to Dolphin's Places
    KFilePlacesModel m_placesModel;
    Mounter *m_mounter;
};

SftpPlugin::SftpPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , d(new Pimpl())
    , deviceId(device()->id())
{
    addToDolphin();
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Created device:" << device()->name();
}

SftpPlugin::~SftpPlugin()
{
    removeFromDolphin();
    unmount();
}

void SftpPlugin::addToDolphin()
{
    removeFromDolphin();

    QUrl kioUrl(QStringLiteral("kdeconnect://") + deviceId + QStringLiteral("/"));
    d->m_placesModel.addPlace(device()->name(), kioUrl, QStringLiteral("kdeconnect"));
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "add to dolphin";
}

void SftpPlugin::removeFromDolphin()
{
    QUrl kioUrl(QStringLiteral("kdeconnect://") + deviceId + QStringLiteral("/"));
    QModelIndex index = d->m_placesModel.closestItem(kioUrl);
    while (index.row() != -1) {
        d->m_placesModel.removePlace(index);
        index = d->m_placesModel.closestItem(kioUrl);
    }
}

void SftpPlugin::mount()
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Mount device:" << device()->name();
    if (d->m_mounter) {
        return;
    }

    d->m_mounter = new Mounter(this);
    connect(d->m_mounter, &Mounter::mounted, this, &SftpPlugin::onMounted);
    connect(d->m_mounter, &Mounter::unmounted, this, &SftpPlugin::onUnmounted);
    connect(d->m_mounter, &Mounter::failed, this, &SftpPlugin::onFailed);
}

void SftpPlugin::unmount()
{
    if (d->m_mounter) {
        d->m_mounter->deleteLater();
        d->m_mounter = nullptr;
    }
}

bool SftpPlugin::mountAndWait()
{
    mount();
    return d->m_mounter->wait();
}

bool SftpPlugin::isMounted() const
{
    return d->m_mounter && d->m_mounter->isMounted();
}

QString SftpPlugin::getMountError()
{
    if (!mountError.isEmpty()) {
        return mountError;
    }
    return QString();
}

bool SftpPlugin::startBrowsing()
{
    if (mountAndWait()) {
        auto *job = new KIO::OpenUrlJob(QUrl(QStringLiteral("kdeconnect://") + deviceId));
        job->setUiDelegate(new KNotificationJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled));
        job->start();
        return true;
    }
    return false;
}

bool SftpPlugin::receivePacket(const NetworkPacket &np)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
    const auto keys = np.body().keys().toSet();
#else
    const QStringList keysList = np.body().keys();
    const auto keys = QSet(keysList.begin(), keysList.end());
#endif
    if (!(fields_c - keys).isEmpty() && !np.has(QStringLiteral("errorMessage"))) {
        // packet is invalid
        return false;
    }

    Q_EMIT packetReceived(np);

    remoteDirectories.clear();
    if (np.has(QStringLiteral("multiPaths"))) {
        QStringList paths = np.get<QStringList>(QStringLiteral("multiPaths"), QStringList());
        QStringList names = np.get<QStringList>(QStringLiteral("pathNames"), QStringList());
        int size = qMin<int>(names.size(), paths.size());
        for (int i = 0; i < size; i++) {
            remoteDirectories.insert(mountPoint() + paths.at(i), names.at(i));
        }
    } else {
        remoteDirectories.insert(mountPoint(), i18n("All files"));
        remoteDirectories.insert(mountPoint() + QStringLiteral("/DCIM/Camera"), i18n("Camera pictures"));
    }
    return true;
}

QString SftpPlugin::mountPoint()
{
    QString runtimePath = QStandardPaths::writableLocation(QStandardPaths::RuntimeLocation);
    if (runtimePath.isEmpty()) {
        runtimePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    }
    return QDir(runtimePath).absoluteFilePath(deviceId);
}

void SftpPlugin::onMounted()
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << device()->name() << QStringLiteral("Remote filesystem mounted at %1").arg(mountPoint());

    Q_EMIT mounted();
}

void SftpPlugin::onUnmounted()
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << device()->name() << "Remote filesystem unmounted";

    unmount();

    Q_EMIT unmounted();
}

void SftpPlugin::onFailed(const QString &message)
{
    mountError = message;
    KNotification::event(KNotification::Error, device()->name(), message);

    unmount();

    Q_EMIT unmounted();
}

QVariantMap SftpPlugin::getDirectories()
{
    return remoteDirectories;
}

#include "sftpplugin.moc"
