/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "shareplugin.h"

#include <QDBusConnection>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QMimeData>
#include <QProcess>
#include <QStandardPaths>
#include <QTemporaryFile>

#include <KApplicationTrader>
#include <KIO/Job>
#include <KIO/MkpathJob>
#include <KIO/OpenUrlJob>
#include <KJobTrackerInterface>
#include <KLocalizedString>
#include <KNotification>
#include <KPluginFactory>
#include <KSystemClipboard>

#include "core/daemon.h"
#include "core/filetransferjob.h"
#include "plugin_share_debug.h"

K_PLUGIN_CLASS_WITH_JSON(SharePlugin, "kdeconnect_share.json")

SharePlugin::SharePlugin(QObject *parent, const QVariantList &args)
    : KdeConnectPlugin(parent, args)
    , m_compositeJob()
{
}

QUrl SharePlugin::destinationDir() const
{
    const QString defaultDownloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QUrl dir = QUrl::fromLocalFile(config()->getString(QStringLiteral("incoming_path"), defaultDownloadPath));

    if (dir.path().contains(QLatin1String("%1"))) {
        dir.setPath(dir.path().arg(device()->name()));
    }

    KJob *job = KIO::mkpath(dir);
    bool ret = job->exec();
    if (!ret) {
        qWarning() << "couldn't create" << dir;
    }

    return dir;
}

QUrl SharePlugin::getFileDestination(const QString filename) const
{
    const QUrl dir = destinationDir().adjusted(QUrl::StripTrailingSlash);
    QUrl destination(dir);
    destination.setPath(dir.path() + QStringLiteral("/") + filename, QUrl::DecodedMode);
    return destination;
}

static QString cleanFilename(const QString &filename)
{
    int idx = filename.lastIndexOf(QLatin1Char('/'));
    return idx >= 0 ? filename.mid(idx + 1) : filename;
}

void SharePlugin::setDateModified(const QUrl &destination, const qint64 timestamp)
{
    QFile receivedFile(destination.toLocalFile());
    if (!receivedFile.exists() || !receivedFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        return;
    }
    receivedFile.setFileTime(QDateTime::fromMSecsSinceEpoch(timestamp), QFileDevice::FileTime(QFileDevice::FileModificationTime));
}

void SharePlugin::setDateCreated(const QUrl &destination, const qint64 timestamp)
{
    QFile receivedFile(destination.toLocalFile());
    if (!receivedFile.exists() || !receivedFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        return;
    }
    receivedFile.setFileTime(QDateTime::fromMSecsSinceEpoch(timestamp), QFileDevice::FileTime(QFileDevice::FileBirthTime));
}

void SharePlugin::receivePacket(const NetworkPacket &np)
{
    /*
        //TODO: Write a test like this
        if (np.type() == PACKET_TYPE_PING) {

            qCDebug(KDECONNECT_PLUGIN_SHARE) << "sending file" << (QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.bashrc");

            NetworkPacket out(PACKET_TYPE_SHARE_REQUEST);
            out.set("filename", mDestinationDir + "itworks.txt");
            AutoClosingQFile* file = new AutoClosingQFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.bashrc"); //Test file to
       transfer

            out.setPayload(file, file->size());

            device()->sendPacket(out);

        }
    */

    qCDebug(KDECONNECT_PLUGIN_SHARE) << "File transfer";

    if (np.hasPayload() || np.has(QStringLiteral("filename"))) {
        //         qCDebug(KDECONNECT_PLUGIN_SHARE) << "receiving file" << filename << "in" << dir << "into" << destination;
        const QString filename = cleanFilename(np.get<QString>(QStringLiteral("filename"), QString::number(QDateTime::currentMSecsSinceEpoch())));
        QUrl destination = getFileDestination(filename);

        if (np.hasPayload()) {
            qint64 dateCreated = np.get<qint64>(QStringLiteral("creationTime"), QDateTime::currentMSecsSinceEpoch());
            qint64 dateModified = np.get<qint64>(QStringLiteral("lastModified"), QDateTime::currentMSecsSinceEpoch());
            const bool open = np.get<bool>(QStringLiteral("open"), false);

            if (!m_compositeJob) {
                m_compositeJob = new CompositeFileTransferJob(device(), this);
                m_compositeJob->setProperty("destUrl", destinationDir().toString());
                m_compositeJob->setProperty("immediateProgressReporting", true);
                Daemon::instance()->jobTracker()->registerJob(m_compositeJob);
            }

            FileTransferJob *job = np.createPayloadTransferJob(destination);
            job->setOriginName(device()->name() + QStringLiteral(": ") + filename);
            job->setAutoRenameIfDestinatinonExists(true);
            connect(job, &KJob::result, this, [this, dateCreated, dateModified, open](KJob *job) -> void {
                finished(job, dateCreated, dateModified, open);
            });
            m_compositeJob->addSubjob(job);

            if (!m_compositeJob->isRunning()) {
                m_compositeJob->start();
            }
        } else {
            QFile file(destination.toLocalFile());
            file.open(QIODevice::WriteOnly);
            file.close();
        }
    } else if (np.has(QStringLiteral("text"))) {
        QString text = np.get<QString>(QStringLiteral("text"));

        auto mimeData = new QMimeData;
        mimeData->setText(text);
        KSystemClipboard::instance()->setMimeData(mimeData, QClipboard::Clipboard);

        QUrl url;
        QStringList lines = text.split(QStringLiteral("\n"), Qt::SkipEmptyParts);
        if (lines.count()) {
            url.setUrl(lines[lines.count() - 1].trimmed());
        }

        KNotification *notif = new KNotification(QStringLiteral("textShareReceived"));
        notif->setComponentName(QStringLiteral("kdeconnect"));
        notif->setText(text);
        notif->setTitle(i18nc("@info Some piece of text was received from a connected device", "Shared text from %1 copied to clipboard", device()->name()));

        auto openTextEditor = [this, text, notif] {
            KService::Ptr service = KApplicationTrader::preferredService(QStringLiteral("text/plain"));
            const QString defaultApp = service ? service->desktopEntryName() : QString();

            if (defaultApp == QLatin1String("org.kde.kate") || defaultApp == QLatin1String("org.kde.kwrite")) {
                QProcess *proc = new QProcess();
                connect(proc, &QProcess::finished, proc, &QObject::deleteLater);
                if (const QString token = notif->xdgActivationToken(); !token.isEmpty()) {
                    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
                    env.insert(QStringLiteral("XDG_ACTIVATION_TOKEN"), token);
                    proc->setProcessEnvironment(env);
                }
                proc->start(defaultApp.section(QStringLiteral("."), 2, 2), QStringList(QStringLiteral("--stdin")));
                proc->write(text.toUtf8());
                proc->closeWriteChannel();
            } else {
                QTemporaryFile tmpFile;
                tmpFile.setFileTemplate(QStringLiteral("kdeconnect-XXXXXX.txt"));
                tmpFile.setAutoRemove(false);
                tmpFile.open();
                tmpFile.write(text.toUtf8());
                tmpFile.close();

                const QString fileName = tmpFile.fileName();
                auto *job = new KIO::OpenUrlJob(QUrl::fromLocalFile(fileName), QStringLiteral("text/plain"));
                job->setStartupId(notif->xdgActivationToken().toUtf8());
                job->start();
                Q_EMIT shareReceived(fileName);
            }
        };

        auto openUrl = [this, url, notif] {
            auto *job = new KIO::OpenUrlJob(url);
            job->setStartupId(notif->xdgActivationToken().toUtf8());
            job->start();
            Q_EMIT shareReceived(url.toString());
        };

        KNotificationAction *textEditorAction = notif->addAction(i18nc("@action:button Edit text with default text editor", "Open in Text Editor"));
        connect(textEditorAction, &KNotificationAction::activated, this, openTextEditor);

        if (url.isValid() && (url.scheme() == QStringLiteral("http") || url.scheme() == QStringLiteral("https"))) {
            KNotificationAction *openLinkAction = notif->addAction(i18nc("@action:button Open URL with default app", "Open Link"));
            connect(openLinkAction, &KNotificationAction::activated, this, openUrl);
        }
        notif->sendEvent();

    } else if (np.has(QStringLiteral("url"))) {
        QUrl url = QUrl::fromEncoded(np.get<QByteArray>(QStringLiteral("url")));
        QDesktopServices::openUrl(url);
        Q_EMIT shareReceived(url.toString());
    } else {
        qCDebug(KDECONNECT_PLUGIN_SHARE) << "Error: Nothing attached!";
    }
}

void SharePlugin::finished(KJob *job, const qint64 dateModified, const qint64 dateCreated, const bool open)
{
    FileTransferJob *ftjob = qobject_cast<FileTransferJob *>(job);
    if (ftjob && !job->error()) {
        Q_EMIT shareReceived(ftjob->destination().toString());
        setDateCreated(ftjob->destination(), dateCreated);
        setDateModified(ftjob->destination(), dateModified);
        qCDebug(KDECONNECT_PLUGIN_SHARE) << "File transfer finished." << ftjob->destination();
        if (open) {
            QDesktopServices::openUrl(ftjob->destination());
        }
    } else {
        qCDebug(KDECONNECT_PLUGIN_SHARE) << "File transfer failed." << (ftjob ? ftjob->destination() : QUrl());
    }
}

void SharePlugin::openDestinationFolder()
{
    QDesktopServices::openUrl(destinationDir());
}

void SharePlugin::shareUrl(const QUrl &url, bool open)
{
    NetworkPacket packet(PACKET_TYPE_SHARE_REQUEST);
    if (url.isLocalFile() && !url.isRelative()) {
        QSharedPointer<QFile> ioFile(new QFile(url.toLocalFile()));

        if (!ioFile->exists()) {
            Daemon::instance()->reportError(i18n("Could not share file"), i18n("%1 does not exist", url.toLocalFile()));
            return;
        } else {
            QFileInfo info(*ioFile);
            packet.setPayload(ioFile, ioFile->size());
            packet.set<QString>(QStringLiteral("filename"), QUrl(url).fileName());
            packet.set<qint64>(QStringLiteral("creationTime"), info.birthTime().toMSecsSinceEpoch());
            packet.set<qint64>(QStringLiteral("lastModified"), info.lastModified().toMSecsSinceEpoch());
            packet.set<bool>(QStringLiteral("open"), open);
        }
    } else {
        packet.set<QString>(QStringLiteral("url"), url.toString());
    }
    sendPacket(packet);
}

void SharePlugin::shareUrls(const QStringList &urls)
{
    for (const QString &url : urls) {
        shareUrl(QUrl(url), false);
    }
}

void SharePlugin::shareText(const QString &text)
{
    NetworkPacket packet(PACKET_TYPE_SHARE_REQUEST);
    packet.set<QString>(QStringLiteral("text"), text);
    sendPacket(packet);
}

QString SharePlugin::dbusPath() const
{
    return QLatin1String("/modules/kdeconnect/devices/%1/share").arg(device()->id());
}

#include "moc_shareplugin.cpp"
#include "shareplugin.moc"
