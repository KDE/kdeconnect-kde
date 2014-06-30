/*
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

#include "filetransferjob.h"

#include <qalgorithms.h>
#include <QFileInfo>

#include <KIO/RenameDialog>
#include <KLocalizedString>

#include "kdebugnamespace.h"

FileTransferJob::FileTransferJob(const QSharedPointer<QIODevice>& origin, int size, const KUrl& destination): KJob()
{
    Q_ASSERT(destination.isLocalFile());
    //TODO: Make a precondition before calling this function that destination file exists
    mOrigin = origin;
    mSize = size;
    mWritten = 0;
    m_speedBytes = 0;
    mDestination = destination;
    mDestinationJob = 0;
    mDeviceName = i18nc("Device name that will appear on the jobs", "KDE-Connect");

    setCapabilities(Killable);
    kDebug(debugArea()) << "FileTransferJob Downloading payload to" << destination;
}

void FileTransferJob::openFinished(KJob* job)
{
    kDebug(debugArea()) << job->errorString();
}

void FileTransferJob::start()
{
    QMetaObject::invokeMethod(this, "doStart", Qt::QueuedConnection);
    //kDebug(debugArea()) << "FileTransferJob start";
}

void FileTransferJob::doStart()
{
    description(this, i18n("Receiving file over KDE-Connect"),
        QPair<QString, QString>(i18nc("File transfer origin", "From"),
        QString(mDeviceName))
    );
    KUrl destCheck = mDestination;
    if (QFile::exists(destCheck.path())) {
        QFileInfo destInfo(destCheck.path());
        KIO::RenameDialog *dialog = new KIO::RenameDialog(0,
            i18n("Incoming file exists"),
            KUrl(mDeviceName + ":/" + destCheck.fileName()),
            destCheck,
            KIO::M_OVERWRITE,
            mSize,
            destInfo.size(),
            -1,
            destInfo.created().toTime_t(),
            -1,
            destInfo.lastModified().toTime_t()
        );
        connect(this, SIGNAL(finished(KJob*)), dialog, SLOT(deleteLater()));
        connect(dialog, SIGNAL(finished(int)), SLOT(renameDone(int)));
        dialog->show();
        return;
    }

    startTransfer();
}

void FileTransferJob::renameDone(int result)
{
    KIO::RenameDialog *renameDialog = qobject_cast<KIO::RenameDialog*>(sender());
    switch (result) {
    case KIO::R_CANCEL:
        //The user cancelled, killing the job
        emitResult();
    case KIO::R_RENAME:
        mDestination = renameDialog->newDestUrl();
        break;
    case KIO::R_OVERWRITE:
    {
        // Delete the old file if exists
        QFile oldFile(mDestination.path());
        if (oldFile.exists()) {
            oldFile.remove();
        }
        break;
    }
    default:
        kWarning() << "Unknown Error";
        emitResult();
    }

    renameDialog->deleteLater();
    startTransfer();
}

void FileTransferJob::startTransfer()
{
    setTotalAmount(Bytes, mSize);
    setProcessedAmount(Bytes, 0);
    m_time = QTime::currentTime();
    description(this, i18n("Receiving file over KDE-Connect"),
                        QPair<QString, QString>(i18nc("File transfer origin", "From"),
                        QString(mDeviceName)),
                        QPair<QString, QString>(i18nc("File transfer destination", "To"), mDestination.path()));

    QFile(mDestination.path()).open(QIODevice::WriteOnly | QIODevice::Truncate); //HACK: KIO is so dumb it can't create the file if it doesn't exist
    mDestinationJob = KIO::open(mDestination, QIODevice::WriteOnly);
    connect(mDestinationJob, SIGNAL(open(KIO::Job*)), this, SLOT(open(KIO::Job*)));
    connect(mDestinationJob, SIGNAL(result(KJob*)), this, SLOT(openFinished(KJob*)));

    //Open destination file
    mDestinationJob->start();
}

void FileTransferJob::open(KIO::Job* job)
{
    Q_UNUSED(job);

    //kDebug(debugArea()) << "FileTransferJob open";

    if (!mOrigin) {
        kDebug(debugArea()) << "FileTransferJob: Origin is null";
        return;
    }

    //Open source file
    mOrigin->open(QIODevice::ReadOnly);
    Q_ASSERT(mOrigin->isOpen());

    connect(mOrigin.data(), SIGNAL(readyRead()),this, SLOT(readyRead()));
    connect(mOrigin.data(), SIGNAL(disconnected()),this, SLOT(sourceFinished()));
    if (mOrigin->bytesAvailable() > 0) readyRead();

}

void FileTransferJob::readyRead()
{
    int bytes = qMin(qint64(4096), mOrigin->bytesAvailable());
    QByteArray data = mOrigin->read(bytes);
    mDestinationJob->write(data);
    mWritten += data.size();
    setProcessedAmount(Bytes, mWritten);

    //kDebug(debugArea()) << "readyRead" << mSize << mWritten << bytes;

    if (mSize > -1) {
        //If a least 1 second has passed since last update
        int secondsSinceLastTime = m_time.secsTo(QTime::currentTime());
        if (secondsSinceLastTime > 0 && m_speedBytes > 0) {
            float speed = (mWritten - m_speedBytes) / secondsSinceLastTime;
            emitSpeed(speed);

            m_time = QTime::currentTime();
            m_speedBytes = mWritten;
        } else if(m_speedBytes == 0) {
            m_speedBytes = mWritten;
        }
    }

    if (mSize > -1 && mWritten >= mSize) { //At the end or expected size reached
        mOrigin->close();
        //sourceFinished();
    } else if (mOrigin->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    }
}

void FileTransferJob::sourceFinished()
{
    //Make sure we do not enter this function again
    disconnect(mOrigin.data(), SIGNAL(aboutToClose()),this, SLOT(sourceFinished()));

    //TODO: MD5 check the file
    if (mSize > -1 && mWritten != mSize) {
        kDebug(debugArea()) << "Received incomplete file (" << mWritten << " of " << mSize << " bytes)";
        setError(1);
        setErrorText(i18n("Received incomplete file"));
    } else {
        kDebug(debugArea()) << "Finished transfer" << mDestinationJob->url();
    }
    mDestinationJob->close();
    mDestinationJob->deleteLater();
    emitResult();
}

bool FileTransferJob::doKill()
{
    if (mDestinationJob) {
        mDestinationJob->close();
    }
    if (mOrigin) {
        mOrigin->close();
    }
    return true;
}
