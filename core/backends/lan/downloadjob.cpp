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

#include "downloadjob.h"

#include <core/core_debug.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include "lanlinkprovider.h"

DownloadJob::DownloadJob(QHostAddress address, QVariantMap transferInfo): KJob()
{
    mAddress = address;
    mPort = transferInfo["port"].toInt();
    mSocket = QSharedPointer<QTcpSocket>(new QTcpSocket());
}

DownloadJob::~DownloadJob()
{
}

void DownloadJob::start()
{
    //TODO: Timeout?
    connect(mSocket.data(), &QAbstractSocket::disconnected, this, &DownloadJob::done);
    connect(mSocket.data(), SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(done()));
    //connect(mSocket.data(), &QAbstractSocket::connected, [=](){ qDebug() << "Connected"; });

    mSocket->connectToHost(mAddress, mPort, QIODevice::ReadOnly);

    //mSocket->open(QIODevice::ReadOnly);

    //TODO: Implement payload encryption somehow (create an intermediate iodevice to encrypt the payload here?)
}

void DownloadJob::done()
{
    if (mSocket->error()) {
        qWarning(KDECONNECT_CORE) << mSocket->errorString();
    }
    emitResult();
    deleteLater();
}

QSharedPointer<QIODevice> DownloadJob::getPayload()
{
    return mSocket.staticCast<QIODevice>();
}
