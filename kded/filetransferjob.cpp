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

#include <KLocalizedString>

#include <QDebug>
#include <qalgorithms.h>

FileTransferJob::FileTransferJob(const QSharedPointer<QIODevice>& origin, int size, const KUrl& destination): KJob()
{
    mDestination = KIO::open(destination, QIODevice::WriteOnly);
    mOrigin = origin;
    mSize = size;
    mWritten = 0;
    qDebug() << "Downloading payload to" << destination;
}

void FileTransferJob::start()
{
    //Open destination file
    connect(mDestination, SIGNAL(open(KIO::Job*)), this, SLOT(open(KIO::Job*)));
    mDestination->start();
}

void FileTransferJob::open(KIO::Job* job)
{
    Q_UNUSED(job);

    qDebug() << "open";

    if (!mOrigin) {
        qDebug() << "FileTransferJob: Origin is null";
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
    qDebug() << "readyRead";

    //Copy a chunk of data

    int bytes = qMin(qint64(4096), mOrigin->bytesAvailable());
    QByteArray data = mOrigin->read(bytes);
    mDestination->write(data);
    mWritten += bytes;

    if (mSize > -1) {
        setPercent((mWritten*100)/mSize);
    }

    qDebug() << mSize << mWritten << bytes;

    if (mSize > -1 && mWritten >= mSize) { //At the end or expected size reached
        qDebug() << "No more data to read";
        disconnect(mOrigin.data(), SIGNAL(readyRead()),this, SLOT(readyRead()));
        mOrigin->close();
    } else if (mOrigin->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
    }
}

void FileTransferJob::sourceFinished()
{
    qDebug() << "sourceFinished";

    //Make sure we do not enter this function again
    disconnect(mOrigin.data(), SIGNAL(aboutToClose()),this, SLOT(sourceFinished()));

    //TODO: MD5 check the file
    if (mSize > -1 && mWritten != mSize) {
        qDebug() << "Received incomplete file";
        setError(1);
        setErrorText(i18n("Received incomplete file"));
        emitResult();
    } else {
        qDebug() << "Finished transfer" << mDestination->url();
    }
    emitResult();
}


