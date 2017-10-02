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

#include "shareplugin.h"
#include "share_debug.h"

#include <QStandardPaths>
#include <QProcess>
#include <QDir>
#include <QDesktopServices>
#include <QDBusConnection>
#include <QDebug>
#include <QTemporaryFile>

#include <KLocalizedString>
#include <KJobTrackerInterface>
#include <KPluginFactory>
#include <KIO/MkpathJob>

#include "core/filetransferjob.h"

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_share.json", registerPlugin< SharePlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SHARE, "kdeconnect.plugin.share")

SharePlugin::SharePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

QUrl SharePlugin::destinationDir() const
{
    const QString defaultDownloadPath = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
    QUrl dir = QUrl::fromLocalFile(config()->get<QString>(QStringLiteral("incoming_path"), defaultDownloadPath));

    if (dir.path().contains(QLatin1String("%1"))) {
        dir.setPath(dir.path().arg(device()->name()));
    }

    KJob* job = KIO::mkpath(dir);
    bool ret = job->exec();
    if (!ret) {
        qWarning() << "couldn't create" << dir;
    }

    return dir;
}

static QString cleanFilename(const QString &filename)
{
    int idx = filename.lastIndexOf(QLatin1Char('/'));
    return idx>=0 ? filename.mid(idx + 1) : filename;
}

bool SharePlugin::receivePackage(const NetworkPackage& np)
{
/*
    //TODO: Write a test like this
    if (np.type() == PACKAGE_TYPE_PING) {

        qCDebug(KDECONNECT_PLUGIN_SHARE) << "sending file" << (QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.bashrc");

        NetworkPackage out(PACKAGE_TYPE_SHARE_REQUEST);
        out.set("filename", mDestinationDir + "itworks.txt");
        AutoClosingQFile* file = new AutoClosingQFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.bashrc"); //Test file to transfer

        out.setPayload(file, file->size());

        device()->sendPackage(out);

        return true;

    }
*/

    qCDebug(KDECONNECT_PLUGIN_SHARE) << "File transfer";

    if (np.hasPayload()) {
        const QString filename = cleanFilename(np.get<QString>(QStringLiteral("filename"), QString::number(QDateTime::currentMSecsSinceEpoch())));
        const QUrl dir = destinationDir().adjusted(QUrl::StripTrailingSlash);
        QUrl destination(dir);
        destination.setPath(dir.path() + '/' + filename, QUrl::DecodedMode);
        if (destination.isLocalFile() && QFile::exists(destination.toLocalFile())) {
            destination.setPath(dir.path() + '/' + KIO::suggestName(dir, filename), QUrl::DecodedMode);
        }
//         qCDebug(KDECONNECT_PLUGIN_SHARE) << "receiving file" << filename << "in" << dir << "into" << destination;

        FileTransferJob* job = np.createPayloadTransferJob(destination);
        job->setOriginName(device()->name() + ": " + filename);
        connect(job, &KJob::result, this, &SharePlugin::finished);
        KIO::getJobTracker()->registerJob(job);
        job->start();
    } else if (np.has(QStringLiteral("text"))) {
        QString text = np.get<QString>(QStringLiteral("text"));
        if (!QStandardPaths::findExecutable(QStringLiteral("kate")).isEmpty()) {
            QProcess* proc = new QProcess();
            connect(proc, SIGNAL(finished(int)), proc, SLOT(deleteLater()));
            proc->start(QStringLiteral("kate"), QStringList(QStringLiteral("--stdin")));
            proc->write(text.toUtf8());
            proc->closeWriteChannel();
        } else {
            QTemporaryFile tmpFile;
            tmpFile.setAutoRemove(false);
            tmpFile.open();
            tmpFile.write(text.toUtf8());
            tmpFile.close();

            const QString fileName = tmpFile.fileName();
            Q_EMIT shareReceived(fileName);
            QDesktopServices::openUrl(QUrl::fromLocalFile(fileName));
        }
    } else if (np.has(QStringLiteral("url"))) {
        QUrl url = QUrl::fromEncoded(np.get<QByteArray>(QStringLiteral("url")));
        QDesktopServices::openUrl(url);
        Q_EMIT shareReceived(url.toString());
    } else {
        qCDebug(KDECONNECT_PLUGIN_SHARE) << "Error: Nothing attached!";
    }

    return true;
}

void SharePlugin::finished(KJob* job)
{
    FileTransferJob* ftjob = qobject_cast<FileTransferJob*>(job);
    if (ftjob && !job->error()) {
        Q_EMIT shareReceived(ftjob->destination().toString());
        qCDebug(KDECONNECT_PLUGIN_SHARE) << "File transfer finished." << ftjob->destination();
    } else {
        qCDebug(KDECONNECT_PLUGIN_SHARE) << "File transfer failed." << (ftjob ? ftjob->destination() : QUrl());
    }
}

void SharePlugin::openDestinationFolder()
{
    QDesktopServices::openUrl(destinationDir());
}

void SharePlugin::shareUrl(const QUrl& url)
{
    NetworkPackage package(PACKAGE_TYPE_SHARE_REQUEST);
    if(url.isLocalFile()) {
        QSharedPointer<QIODevice> ioFile(new QFile(url.toLocalFile()));
        package.setPayload(ioFile, ioFile->size());
        package.set<QString>(QStringLiteral("filename"), QUrl(url).fileName());
    } else {
        package.set<QString>(QStringLiteral("url"), url.toString());
    }
    sendPackage(package);
}

QString SharePlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/share";
}

#include "shareplugin.moc"
