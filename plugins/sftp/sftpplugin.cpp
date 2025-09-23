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

#include <KIO/OpenUrlJob>
#include <KLocalizedString>
#include <KNotification>
#include <KNotificationJobUiDelegate>
#include <KPluginFactory>

#include "daemon.h"
#include "mounter.h"
#include "plugin_sftp_debug.h"

K_PLUGIN_CLASS_WITH_JSON(SftpPlugin, "kdeconnect_sftp.json")

SftpPlugin::SftpPlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_mounter(nullptr)
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
    m_placesModel.addPlace(device()->name(), kioUrl, QStringLiteral("kdeconnect"));
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "add to dolphin";
}

void SftpPlugin::removeFromDolphin()
{
    QUrl kioUrl(QStringLiteral("kdeconnect://") + deviceId + QStringLiteral("/"));
    for (int i = 0; i < m_placesModel.rowCount(); ++i) {
        QModelIndex index = m_placesModel.index(i, 0);
        QUrl url = m_placesModel.url(index);
        if (url == kioUrl) {
            m_placesModel.removePlace(index);
            --i;
        }
    }
}

void SftpPlugin::mount()
{
    qCDebug(KDECONNECT_PLUGIN_SFTP) << "Mount device:" << device()->name();
    if (m_mounter) {
        return;
    }

    m_mounter = new Mounter(this);
    connect(m_mounter, &Mounter::mounted, this, &SftpPlugin::onMounted);
    connect(m_mounter, &Mounter::unmounted, this, &SftpPlugin::onUnmounted);
    connect(m_mounter, &Mounter::failed, this, &SftpPlugin::onFailed);
}

void SftpPlugin::unmount()
{
    if (m_mounter) {
        m_mounter->deleteLater();
        m_mounter = nullptr;
    }
}

bool SftpPlugin::mountAndWait()
{
    mount();
    return m_mounter->wait();
}

bool SftpPlugin::isMounted() const
{
    return m_mounter && m_mounter->isMounted();
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

void SftpPlugin::receivePacket(const NetworkPacket &np)
{
    if (np.has(QStringLiteral("errorMessage"))) {
        onFailed(np.get<QString>(QStringLiteral("errorMessage")));
        return;
    }

    // Note: for Windows, a password field is also expected. See the expectedFields definition in sftpplugin-win.cpp
    static const QSet<QString> expectedFields{QStringLiteral("user"), QStringLiteral("port"), QStringLiteral("path"), QStringLiteral("password")};
    const QStringList receivedFieldsList = np.body().keys();
    const QSet<QString> receivedFields(receivedFieldsList.begin(), receivedFieldsList.end());
    if (!(expectedFields - receivedFields).isEmpty()) {
        qCWarning(KDECONNECT_PLUGIN_SFTP) << "Invalid sftp packet received";
        return;
    }

    // if a packet arrives before mounting or after the mount timed out, ignore it
    if (!m_mounter) {
        qCDebug(KDECONNECT_PLUGIN_SFTP) << "Received network packet but no mount is active, ignoring";
        return;
    }

    m_mounter->onPacketReceived(np);

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
    Daemon::instance()->reportError(device()->name(), message);
    unmount();

    Q_EMIT unmounted();
}

QVariantMap SftpPlugin::getDirectories()
{
    return remoteDirectories;
}

#include "moc_sftpplugin.cpp"
#include "sftpplugin.moc"
