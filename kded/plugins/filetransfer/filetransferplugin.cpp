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

#include "filetransferplugin.h"

#include <KIcon>
#include <KLocalizedString>
#include <KStandardDirs>

#include <QDebug>
#include <QFile>
#include <qprocess.h>
#include <QDesktopServices>

#include "../../filetransferjob.h"
#include "autoclosingqfile.h"

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< FileTransferPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_filetransfer", "kdeconnect_filetransfer") )

FileTransferPlugin::FileTransferPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    //TODO: Use downloads user path
    //TODO: Be able to change this from config
    mDestinationDir = QDesktopServices::storageLocation(QDesktopServices::DesktopLocation);
    if (!mDestinationDir.endsWith('/')) mDestinationDir.append('/');
}

bool FileTransferPlugin::receivePackage(const NetworkPackage& np)
{
/*
    //TODO: Move this code to a test and add a diff between files
    if (np.type() == PACKAGE_TYPE_PING) {

        qDebug() << "sending file" << (QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.bashrc");

        NetworkPackage out(PACKAGE_TYPE_FILETRANSFER);
        out.set("filename", mDestinationDir + "itworks.txt");
         //TODO: Use shared pointers
        AutoClosingQFile* file = new AutoClosingQFile(QDesktopServices::storageLocation(QDesktopServices::HomeLocation) + "/.bashrc"); //Test file to transfer

        out.setPayload(file, file->size());

        device()->sendPackage(out);

        return true;

    }
*/

    if (np.type() != PACKAGE_TYPE_FILETRANSFER) return false;
    qDebug() << "File transfer";

    if (np.hasPayload()) {
        //qDebug() << "receiving file";
        QString filename = np.get<QString>("filename", QString::number(QDateTime::currentMSecsSinceEpoch()));
        //TODO: Ask before overwritting or rename file if it already exists
        FileTransferJob* job = np.createPayloadTransferJob(mDestinationDir + filename);
        connect(job, SIGNAL(result(KJob*)), this, SLOT(finished(KJob*)));
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
        qDebug() << "Error: Nothing attached!";
    }

    return true;

}

void FileTransferPlugin::finished(KJob* job)
{
    qDebug() << "File transfer finished";

    FileTransferJob* transferJob = (FileTransferJob*)job;
    KNotification* notification = new KNotification("pingReceived"); //KNotification::Persistent
    notification->setPixmap(KIcon("dialog-ok").pixmap(48, 48));
    notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
    notification->setTitle(i18n("Transfer finished"));
    notification->setText(transferJob->destination().fileName());
    notification->setActions(QStringList(i18n("Open destination folder")));
    connect(notification, SIGNAL(action1Activated()), this, SLOT(openDestinationFolder()));
    notification->sendEvent();
}

void FileTransferPlugin::openDestinationFolder()
{
    QDesktopServices::openUrl(mDestinationDir);
}
