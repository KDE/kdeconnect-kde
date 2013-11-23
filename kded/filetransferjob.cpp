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

#include <KLocalizedString>

#include "kdebugnamespace.h"

FileTransferJob::FileTransferJob(const QSharedPointer<QIODevice>& origin, int size, const KUrl& destination): KJob()
{
    Q_ASSERT(destination.isLocalFile());
    //TODO: Make a precondition before calling this function that destination file exists
    QFile(destination.path()).open(QIODevice::WriteOnly | QIODevice::Truncate); //HACK: KIO is so dumb it can't create the file if it doesn't exist
    mDestination = KIO::open(destination, QIODevice::WriteOnly);
    connect(mDestination, SIGNAL(open(KIO::Job*)), this, SLOT(open(KIO::Job*)));
    connect(mDestination, SIGNAL(result(KJob*)), this, SLOT(openFinished(KJob*)));
    mOrigin = origin;
    mSize = size;
    mWritten = 0;
    kDebug(kdeconnect_kded()) << "FileTransferJob Downloading payload to" << destination;
}

void FileTransferJob::openFinished(KJob* job)
{
    kDebug(kdeconnect_kded()) << job->errorString();
}

void FileTransferJob::start()
{
    //kDebug(kdeconnect_kded()) << "FileTransferJob start";

    //Open destination file
    mDestination->start();
}

void FileTransferJob::open(KIO::Job* job)
{
    Q_UNUSED(job);

    //kDebug(kdeconnect_kded()) << "FileTransferJob open";

    if (!mOrigin) {
        kDebug(kdeconnect_kded()) << "FileTransferJob: Origin is null";
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
    mDestination->write(data);
    mWritten += bytes;

    //kDebug(kdeconnect_kded()) << "readyRead" << mSize << mWritten << bytes;

    if (mSize > -1) {
        setPercent((mWritten*100)/mSize);
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
        kDebug(kdeconnect_kded()) << "Received incomplete file (" << mWritten << " of " << mSize << " bytes)";
        setError(1);
        setErrorText(i18n("Received incomplete file"));
    } else {
        kDebug(kdeconnect_kded()) << "Finished transfer" << mDestination->url();
    }
    mDestination->close();
    mDestination->deleteLater();
    emitResult();
}

