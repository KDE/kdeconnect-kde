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
#include <QDesktopServices>

#include "filetransferjob.h"
#include "autoclosingqfile.h"

K_PLUGIN_FACTORY( KdeConnectPluginFactory, registerPlugin< FileTransferPlugin >(); )
K_EXPORT_PLUGIN( KdeConnectPluginFactory("kdeconnect_filetransfer", "kdeconnect_filetransfer") )

FileTransferPlugin::FileTransferPlugin(QObject* parent, const QVariantList& args)
    : KdeConnectPlugin(parent, args)
{
    //TODO: Use downloads user path
    //TODO: Be able to change this from config
    mDestinationDir = QDesktopServices::DesktopLocation;
}

bool FileTransferPlugin::receivePackage(const NetworkPackage& np)
{

    //TODO: Move this code to a test and do a diff between files
    /*
    if (np.type() == PACKAGE_TYPE_PING) {

        NetworkPackage np(PACKAGE_TYPE_FILETRANSFER);
        np.set("filename", mDestinationDir + "/itworks.txt");
         //TODO: Use shared pointers
        AutoClosingQFile* file = new AutoClosingQFile("/home/vaka/KdeConnect.apk"); //Test file to transfer

        np.setPayload(file);

        device()->sendPackage(np);

        return true;
    }
    */

    if (np.type() != PACKAGE_TYPE_FILETRANSFER) return false;

    if (np.hasPayload()) {
        QString filename = np.get<QString>("filename");
        QIODevice* incoming = np.payload();
        FileTransferJob* job = new FileTransferJob(incoming,filename);
        connect(job,SIGNAL(result(KJob*)), this, SLOT(finished(KJob*)));
        job->start();
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
    //TODO: Add action "open destination folder"
    notification->sendEvent();

}
