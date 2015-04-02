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

#include <KLocalizedString>
#include <KJobTrackerInterface>
#include <KPluginFactory>
#include <KIO/MkpathJob>

#include <core/filetransferjob.h>
#include "autoclosingqfile.h"

K_PLUGIN_FACTORY_WITH_JSON( KdeConnectPluginFactory, "kdeconnect_share.json", registerPlugin< SharePlugin >(); )

Q_LOGGING_CATEGORY(KDECONNECT_PLUGIN_SHARE, "kdeconnect.plugin.share");

SharePlugin::SharePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

QUrl SharePlugin::destinationDir() const
{
    const QUrl defaultDownloadPath = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation));
    QUrl dir = config()->get<QUrl>("incoming_path", defaultDownloadPath);

    if (dir.path().contains("%1")) {
        dir.setPath(dir.path().arg(device()->name()));
    }

    KJob* job = KIO::mkpath(dir);
    bool ret = job->exec();
    if (!ret) {
        qWarning() << "couldn't create" << dir;
    }

    return dir;
}

bool SharePlugin::receivePackage(const NetworkPackage& np)
{
/*
    //TODO: Write a test like this
    if (np.type() == PACKAGE_TYPE_PING) {

        qCDebug(KDECONNECT_PLUGIN_SHARE) << "sending file" << (QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.bashrc");

        NetworkPackage out(PACKAGE_TYPE_SHARE);
        out.set("filename", mDestinationDir + "itworks.txt");
        AutoClosingQFile* file = new AutoClosingQFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.bashrc"); //Test file to transfer

        out.setPayload(file, file->size());

        device()->sendPackage(out);

        return true;

    }
*/

    qCDebug(KDECONNECT_PLUGIN_SHARE) << "File transfer";

    if (np.hasPayload()) {
        //qCDebug(KDECONNECT_PLUGIN_SHARE) << "receiving file";
        const QString filename = np.get<QString>("filename", QString::number(QDateTime::currentMSecsSinceEpoch()));
        const QUrl dir = destinationDir().adjusted(QUrl::StripTrailingSlash);
        QUrl destination(dir.toString() + '/' + filename);
        if (destination.isLocalFile() && QFile::exists(destination.toLocalFile())) {
            destination = QUrl(dir.toString() + '/' + KIO::suggestName(dir, filename));
        }

        FileTransferJob* job = np.createPayloadTransferJob(destination);
        job->setDeviceName(device()->name());
        connect(job, SIGNAL(result(KJob*)), this, SLOT(finished(KJob*)));
        KIO::getJobTracker()->registerJob(job);
        job->start();
    } else if (np.has("text")) {
        QString text = np.get<QString>("text");
        if (!QStandardPaths::findExecutable("kate").isEmpty()) {
            QProcess* proc = new QProcess();
            connect(proc, SIGNAL(finished(int)), proc, SLOT(deleteLater()));
            proc->start("kate", QStringList("--stdin"));
            proc->write(text.toUtf8());
            proc->closeWriteChannel();
        } else {
            QTemporaryFile tmpFile;
            tmpFile.setAutoRemove(false);
            tmpFile.open();
            tmpFile.write(text.toUtf8());
            tmpFile.close();
            QDesktopServices::openUrl(QUrl::fromLocalFile(tmpFile.fileName()));
        }
    } else if (np.has("url")) {
        QUrl url = QUrl::fromEncoded(np.get<QByteArray>("url"));
        QDesktopServices::openUrl(url);
    } else {
        qCDebug(KDECONNECT_PLUGIN_SHARE) << "Error: Nothing attached!";
    }

    return true;

}

void SharePlugin::finished(KJob* job)
{
    qCDebug(KDECONNECT_PLUGIN_SHARE) << "File transfer finished";

    bool error = (job->error() != 0);

    FileTransferJob* transferJob = (FileTransferJob*)job;
    KNotification* notification = new KNotification("transferReceived");
    notification->setIconName(error ? QStringLiteral("dialog-error") : QStringLiteral("dialog-ok"));
    notification->setComponentName("kdeconnect");
    notification->setTitle(error ? i18n("Transfer Failed") : i18n("Transfer Finished"));
    notification->setText(transferJob->destination().fileName());
    notification->setActions(QStringList(i18n("Open")));
    connect(notification, &KNotification::action1Activated, this, &SharePlugin::openDestinationFolder);
    notification->sendEvent();
}

void SharePlugin::openDestinationFolder()
{
    QDesktopServices::openUrl(destinationDir());
}

void SharePlugin::shareUrl(const QUrl& url)
{
    NetworkPackage package(PACKAGE_TYPE_SHARE);
    if(url.isLocalFile()) {
        QSharedPointer<QIODevice> ioFile(new QFile(url.toLocalFile()));
        package.setPayload(ioFile, ioFile->size());
        package.set<QString>("filename", QUrl(url).fileName());
    } else {
        package.set<QString>("url", url.toString());
    }
    sendPackage(package);
}

void SharePlugin::connected()
{
    QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportAllContents);
}

QString SharePlugin::dbusPath() const
{
    return "/modules/kdeconnect/devices/" + device()->id() + "/share";
}

#include "shareplugin.moc"
