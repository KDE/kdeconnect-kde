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

#include <kdeconnectconfig.h>
#include "downloadjob.h"

#include <core/core_debug.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

#include "lanlinkprovider.h"

DownloadJob::DownloadJob(const QHostAddress &address, const QVariantMap &transferInfo): KJob()
{
    mAddress = address;
    mPort = transferInfo["port"].toInt();
    mSocket = QSharedPointer<QSslSocket>(new QSslSocket);
    useSsl = transferInfo.value("useSsl", false).toBool();

    // Setting ssl related properties for socket when using ssl
    if (useSsl) {
        mSocket->setLocalCertificate(KdeConnectConfig::instance()->certificate());
        mSocket->setPrivateKey(KdeConnectConfig::instance()->privateKeyPath());
        mSocket->setProtocol(QSsl::TlsV1_2);
        mSocket->setPeerVerifyName(transferInfo.value("deviceId").toString());
        mSocket->setPeerVerifyMode(QSslSocket::VerifyPeer);
        mSocket->addCaCertificate(QSslCertificate(KdeConnectConfig::instance()->getTrustedDevice(
                transferInfo.value("deviceId").toString()).certificate.toLatin1()));
    }
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

    if (useSsl) {
        // Cannot use read only, might be due to ssl handshake, getting QIODevice::ReadOnly error and no connection
        mSocket->connectToHostEncrypted(mAddress.toString(), mPort, QIODevice::ReadWrite);
        mSocket->waitForEncrypted();
    } else {
        mSocket->connectToHost(mAddress, mPort, QIODevice::ReadOnly);
        mSocket->waitForConnected();
    }

    //mSocket->open(QIODevice::ReadOnly);

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
