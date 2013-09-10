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

#include <QDebug>
#include <qalgorithms.h>

FileTransferJob::FileTransferJob(QIODevice* origin, const KUrl& destination): KJob()
{
    mDestination = destination;
    mOrigin = origin;
}

void FileTransferJob::start()
{
    //Open destination file

    QTemporaryFile tmp;
    tmp.setAutoRemove(false);
    tmp.open();

    mTempDestination = KIO::open(tmp.fileName(), QIODevice::WriteOnly);
    connect(mTempDestination, SIGNAL(open(KIO::Job*)), this, SLOT(open(KIO::Job*)));
    mTempDestination->start();

}

void FileTransferJob::open(KIO::Job* job)
{
    Q_UNUSED(job);
    
    //Open source file

    mOrigin->open(QIODevice::ReadOnly);

    Q_ASSERT(mOrigin->isOpen());

    connect(mOrigin, SIGNAL(readyRead()),this, SLOT(readyRead()));
    connect(mOrigin, SIGNAL(aboutToClose()),this, SLOT(sourceFinished()));
    if (mOrigin->bytesAvailable() > 0) readyRead();

}

void FileTransferJob::readyRead()
{
    //Copy a chunk of data

    int bytes = qMin(qint64(4096), mOrigin->bytesAvailable());
    QByteArray data = mOrigin->read(bytes);
    mTempDestination->write(data);

    if(!mOrigin->atEnd())
        QMetaObject::invokeMethod(this, "readyRead", Qt::QueuedConnection);
}

void FileTransferJob::sourceFinished()
{
    qDebug() << "sourceFinished";

    readyRead(); //Read the last chunk of data, if any

    qDebug() << "Finished" << mTempDestination->url();
    KIO::FileCopyJob* job = KIO::file_move(mTempDestination->url(), mDestination);
    connect(job, SIGNAL(result(KJob*)), this, SLOT(moveResult(KJob*)));
    job->start();

    delete mOrigin; //TODO: Use shared pointers
}

void FileTransferJob::moveResult(KJob* job)
{
    //TODO: Error handling, cleanup
    qDebug() << "Move result";
    qDebug() << job->errorString();
    qDebug() << job->errorText();
    emitResult();
}

