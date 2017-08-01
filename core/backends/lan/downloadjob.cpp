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

#ifndef Q_OS_WIN
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#endif

#include "kdeconnectconfig.h"
#include "lanlinkprovider.h"
#include "core/core_debug.h"

class QQSslSocket : public QSslSocket
{
private:
    // Workaround Qt bug https://codereview.qt-project.org/#/c/195723/
    qint64 readData(char *data, qint64 maxSize) override {
        qint64 res = QSslSocket::readData(data, maxSize);
        if (res == 0 && encryptedBytesAvailable() == 0 && state() != ConnectedState) {
            return maxSize ? qint64(-1) : qint64(0);
        } else {
            return res;
        }
    }
};

DownloadJob::DownloadJob(const QHostAddress &address, const QVariantMap &transferInfo)
    : KJob()
    , mAddress(address)
    , mPort(transferInfo[QStringLiteral("port")].toInt())
    , mSocket(new QQSslSocket)
{
    LanLinkProvider::configureSslSocket(mSocket.data(), transferInfo.value(QStringLiteral("deviceId")).toString(), true);

    connect(mSocket.data(), SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketFailed(QAbstractSocket::SocketError)));
    connect(mSocket.data(), &QAbstractSocket::connected, this, &DownloadJob::socketConnected);
}

DownloadJob::~DownloadJob()
{

}

void DownloadJob::start()
{
    //TODO: Timeout?
    // Cannot use read only, might be due to ssl handshake, getting QIODevice::ReadOnly error and no connection
    mSocket->connectToHostEncrypted(mAddress.toString(), mPort, QIODevice::ReadWrite);
}

void DownloadJob::socketFailed(QAbstractSocket::SocketError error)
{
    qWarning() << "error..." << (error == QAbstractSocket::RemoteHostClosedError) << mSocket->errorString();
    setError(error + 1);
    setErrorText(mSocket->errorString());
    emitResult();
}

QSharedPointer<QIODevice> DownloadJob::getPayload()
{
    return mSocket.staticCast<QIODevice>();
}

void DownloadJob::socketConnected()
{
    emitResult();
}
