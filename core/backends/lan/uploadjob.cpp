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

#include "uploadjob.h"

#include <KLocalizedString>

#include "lanlinkprovider.h"
#include "kdeconnectconfig.h"
#include "core_debug.h"

UploadJob::UploadJob(const QSharedPointer<QIODevice>& source, const QString& deviceId)
    : KJob()
    , mInput(source)
    , mServer(new Server(this))
    , mSocket(nullptr)
    , mPort(0)
    , mDeviceId(deviceId) // We will use this info if link is on ssl, to send encrypted payload
{
    connect(mInput.data(), &QIODevice::readyRead, this, &UploadJob::startUploading);
    connect(mInput.data(), &QIODevice::aboutToClose, this, &UploadJob::aboutToClose);
}

void UploadJob::start()
{
    mPort = MIN_PORT;
    while (!mServer->listen(QHostAddress::Any, mPort)) {
        mPort++;
        if (mPort > MAX_PORT) { //No ports available?
            qCWarning(KDECONNECT_CORE) << "Error opening a port in range" << MIN_PORT << "-" << MAX_PORT;
            mPort = 0;
            setError(1);
            setErrorText(i18n("Couldn't find an available port"));
            emitResult();
            return;
        }
    }
    connect(mServer, &QTcpServer::newConnection, this, &UploadJob::newConnection);
}

void UploadJob::newConnection()
{
    if (!mInput->open(QIODevice::ReadOnly)) {
        qCWarning(KDECONNECT_CORE) << "error when opening the input to upload";
        return; //TODO: Handle error, clean up...
    }

    Server* server = qobject_cast<Server*>(sender());
    // FIXME : It is called again when payload sending is finished. Unsolved mystery :(
    disconnect(mServer, &QTcpServer::newConnection, this, &UploadJob::newConnection);

    mSocket = server->nextPendingConnection();
    mSocket->setParent(this);
    connect(mSocket, &QSslSocket::disconnected, this, &UploadJob::cleanup);
    connect(mSocket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketFailed(QAbstractSocket::SocketError)));
    connect(mSocket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(mSocket, &QSslSocket::encrypted, this, &UploadJob::startUploading);
//     connect(mSocket, &QAbstractSocket::stateChanged, [](QAbstractSocket::SocketState state){ qDebug() << "statechange" << state; });

    LanLinkProvider::configureSslSocket(mSocket, mDeviceId, true);

    mSocket->startServerEncryption();
}

void UploadJob::startUploading()
{
    while ( mInput->bytesAvailable() > 0 )
    {
        qint64 bytes = qMin(mInput->bytesAvailable(), (qint64)4096);
        int w = mSocket->write(mInput->read(bytes));
        if (w<0) {
            qCWarning(KDECONNECT_CORE) << "error when writing data to upload" << bytes << mInput->bytesAvailable();
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
//     qDebug() << "closing...";
    mSocket->disconnectFromHost();
}

void UploadJob::cleanup()
{
    mSocket->close();
//     qDebug() << "closed!";
    emitResult();
}

QVariantMap UploadJob::transferInfo()
{
    Q_ASSERT(mPort != 0);
    return {{"port", mPort}};
}

void UploadJob::socketFailed(QAbstractSocket::SocketError error)
{
    qWarning() << "error uploading" << error;
    setError(2);
    emitResult();
    mSocket->close();
}

void UploadJob::sslErrors(const QList<QSslError>& errors)
{
    qWarning() << "ssl errors" << errors;
    setError(1);
    emitResult();
    mSocket->close();
}
