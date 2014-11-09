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

#include <qalgorithms.h>

#include "kdebugnamespace.h"
#include "uploadjob.h"

UploadJob::UploadJob(const QSharedPointer<QIODevice>& source): KJob()
{
    mInput = source;
    mServer = new QTcpServer(this);
    mSocket = 0;

    connect(mInput.data(), SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(mInput.data(), SIGNAL(aboutToClose()), this, SLOT(aboutToClose()));
}

void UploadJob::start()
{
    mPort = 1739;
    while(!mServer->listen(QHostAddress::Any, mPort)) {
        mPort++;
        if (mPort > 1764) { //No ports available?
            kDebug(debugArea()) << "Error opening a port in range 1739-1764 for file transfer";
            mPort = 0;
            return;
        }
    }
    connect(mServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

void UploadJob::newConnection()
{

    if (mSocket || !mServer->hasPendingConnections()) return;

    if (!mInput->open(QIODevice::ReadOnly)) {
        qWarning() << "error when opening the input to upload";
        return; //TODO: Handle error, clean up...
    }

    mSocket = mServer->nextPendingConnection();
    readyRead();
}

void UploadJob::readyRead()
{
    //TODO: Implement payload encryption

    while ( mInput->bytesAvailable() > 0 )
    {
        qint64 bytes = qMin(mInput->bytesAvailable(), (qint64)4096);
        int w = mSocket->write(mInput->read(bytes));
        if (w<0) {
            qWarning() << "error when writing data to upload" << bytes << mInput->bytesAvailable();
            break;
        }
        else
        {
            while ( mSocket->flush() );
        }
    }

    mInput->close();
}

void UploadJob::aboutToClose()
{
    mSocket->disconnectFromHost();
    mSocket->close();
    emitResult();
}

QVariantMap UploadJob::getTransferInfo()
{
    QVariantMap ret;

    ret["port"] = mPort;

    return ret;
}

