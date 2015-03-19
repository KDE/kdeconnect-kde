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
#include <core_debug.h>

#include <qalgorithms.h>
#include <QFileInfo>
#include <QDebug>

#include <KLocalizedString>

FileTransferJob::FileTransferJob(const QSharedPointer<QIODevice>& origin, qint64 size, const QUrl& destination)
    : KJob()
    , mOrigin(origin)
    , mDestinationJob(0)
    , mDeviceName("KDE Connect")
    , mDestination(destination)
    , mSpeedBytes(0)
    , mSize(size)
    , mWritten(0)
{
    if (mDestination.scheme().isEmpty()) {
        qWarning() << "Destination QUrl" << mDestination << "lacks a scheme. Setting its scheme to 'file'.";
        mDestination.setScheme("file");
    }

    Q_ASSERT(destination.isLocalFile());
    setCapabilities(Killable);
    qCDebug(KDECONNECT_CORE) << "FileTransferJob Downloading payload to" << destination;
}

void FileTransferJob::openFinished(KJob* job)
{
    qCDebug(KDECONNECT_CORE) << job->errorString();
}

void FileTransferJob::start()
{
    QMetaObject::invokeMethod(this, "doStart", Qt::QueuedConnection);
    //qCDebug(KDECONNECT_CORE) << "FileTransferJob start";
}

void FileTransferJob::doStart()
{
    description(this, i18n("Receiving file over KDE-Connect"),
        QPair<QString, QString>(i18nc("File transfer origin", "From"),
        QString(mDeviceName))
    );
    QUrl destCheck = mDestination;
    if (destCheck.isLocalFile() && QFile::exists(destCheck.toLocalFile())) {
        setError(2);
        setErrorText(i18n("Filename already present"));
        emitResult();
    }

    startTransfer();
}

void FileTransferJob::startTransfer()
{
    setTotalAmount(Bytes, mSize);
    setProcessedAmount(Bytes, 0);
    mTime = QTime::currentTime();
    description(this, i18n("Receiving file over KDE-Connect"),
                        QPair<QString, QString>(i18nc("File transfer origin", "From"),
                        QString(mDeviceName)),
                        QPair<QString, QString>(i18nc("File transfer destination", "To"), mDestination.toLocalFile()));

    mDestinationJob = KIO::open(mDestination, QIODevice::WriteOnly);
    QFile(mDestination.toLocalFile()).open(QIODevice::WriteOnly | QIODevice::Truncate); //KIO won't create the file if it doesn't exist
    connect(mDestinationJob, SIGNAL(open(KIO::Job*)), this, SLOT(open(KIO::Job*)));
    connect(mDestinationJob, SIGNAL(result(KJob*)), this, SLOT(openFinished(KJob*)));

    //Open destination file
    mDestinationJob->start();
}

void FileTransferJob::open(KIO::Job* job)
{
    Q_UNUSED(job);

    //qCDebug(KDECONNECT_CORE) << "FileTransferJob open";

    if (!mOrigin) {
        qCDebug(KDECONNECT_CORE) << "FileTransferJob: Origin is null";
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

    //qCDebug(KDECONNECT_CORE) << "readyRead" << mSize << mWritten << bytes;

    if (mSize > -1) {
        //If a least 1 second has passed since last update
        int secondsSinceLastTime = mTime.secsTo(QTime::currentTime());
        if (secondsSinceLastTime > 0 && mSpeedBytes > 0) {
            float speed = (mWritten - mSpeedBytes) / secondsSinceLastTime;
            emitSpeed(speed);

            mTime = QTime::currentTime();
            mSpeedBytes = mWritten;
        } else if(mSpeedBytes == 0) {
            mSpeedBytes = mWritten;
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

    //TODO: MD5-check the file
    if (mSize > -1 && mWritten != mSize) {
        qCDebug(KDECONNECT_CORE) << "Received incomplete file (" << mWritten << " of " << mSize << " bytes)";
        setError(1);
        setErrorText(i18n("Received incomplete file"));
    } else {
        qCDebug(KDECONNECT_CORE) << "Finished transfer" << mDestinationJob->url();
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
