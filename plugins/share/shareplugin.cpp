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

#include <KGlobalSettings>
#include <KIcon>
#include <KLocalizedString>
#include <KStandardDirs>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KJobTrackerInterface>

#include <qprocess.h>
#include <QDir>
#include <QDesktopServices>
#include <QDBusConnection>

#include <core/kdebugnamespace.h>
#include <core/filetransferjob.h>
#include "autoclosingqfile.h"

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< SharePlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_share", "kdeconnect-plugins") )

SharePlugin::SharePlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
}

KUrl SharePlugin::destinationDir() const
{
    //FIXME: There should be a better way to listen to changes in the config file instead of reading the value each time
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnect/plugins/share");
    KUrl dir = config->group("receive").readEntry("path", KGlobalSettings::downloadPath());
    dir.adjustPath(KUrl::AddTrailingSlash);

    QString url = dir.toLocalFile();
    if (url.contains("%1")) url = url.arg(device()->name());

    kDebug(kdeconnect_kded()) << url;
    QDir().mkpath(url);

    return url;
}

bool SharePlugin::receivePackage(const NetworkPackage& np)
{
/*
    //TODO: Use this code to write a test
    if (np.type() == PACKAGE_TYPE_PING) {

        kDebug(kdeconnect_kded()) << "sending file" << (QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.bashrc");

        NetworkPackage out(PACKAGE_TYPE_SHARE);
        out.set("filename", mDestinationDir + "itworks.txt");
         //TODO: Use shared pointers
        AutoClosingQFile* file = new AutoClosingQFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.bashrc"); //Test file to transfer

        out.setPayload(file, file->size());

        device()->sendPackage(out);

        return true;

    }
*/

    kDebug(kdeconnect_kded()) << "File transfer";

    if (np.hasPayload()) {
        //kDebug(kdeconnect_kded()) << "receiving file";
        QString filename = np.get<QString>("filename", QString::number(QDateTime::currentMSecsSinceEpoch()));
        KUrl destination = destinationDir();
        destination.addPath(filename);
        FileTransferJob* job = np.createPayloadTransferJob(destination);
        job->setDeviceName(device()->name());
        connect(job, SIGNAL(result(KJob*)), this, SLOT(finished(KJob*)));
        KIO::getJobTracker()->registerJob(job);
        job->start();
    } else if (np.has("text")) {
        QString text = np.get<QString>("text");
        if (!KStandardDirs::findExe("kate").isEmpty()) {
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
        QUrl url(np.get<QString>("url"));
        QDesktopServices::openUrl(url);
    } else {
        kDebug(kdeconnect_kded()) << "Error: Nothing attached!";
    }

    return true;

}

void SharePlugin::finished(KJob* job)
{
    kDebug(kdeconnect_kded()) << "File transfer finished";

    bool error = (job->error() != 0);

    FileTransferJob* transferJob = (FileTransferJob*)job;
    KNotification* notification = new KNotification("pingReceived"); //KNotification::Persistent
    notification->setPixmap(KIcon(error? "edit-delete" : "dialog-ok").pixmap(48, 48));
    notification->setComponentData(KComponentData("kdeconnect", "kdeconnect-kded"));
    notification->setTitle(i18n("Transfer finished"));
    notification->setText(transferJob->destination().fileName());
    notification->setActions(QStringList(i18n("Open destination folder")));
    connect(notification, SIGNAL(action1Activated()), this, SLOT(openDestinationFolder()));
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
        package.set<QString>("filename", KUrl(url).fileName());
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
