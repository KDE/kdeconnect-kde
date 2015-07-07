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

DownloadJob::DownloadJob(QHostAddress address, QVariantMap transferInfo): KJob()
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

void DownloadJob::start()
{
    //kDebug(kdeconnect_kded()) << "DownloadJob Start";
    if (useSsl) {
        qDebug() << "Connecting to host encrypted";
        // Cannot use read only, might be due to ssl handshake, getting QIODevice::ReadOnly error and no connection
        mSocket->connectToHostEncrypted(mAddress.toString(), mPort, QIODevice::ReadWrite);
        mSocket->waitForEncrypted();
    } else {
        qDebug() << "Connectiong to host unencrypted";
        mSocket->connectToHost(mAddress, mPort, QIODevice::ReadOnly);
        mSocket->waitForConnected();
    }
    connect(mSocket.data(), SIGNAL(disconnected()),
            this, SLOT(disconnected()));
}

void DownloadJob::disconnected()
{
    //kDebug(kdeconnect_kded()) << "DownloadJob End";
    emitResult();
}

QSharedPointer<QIODevice> DownloadJob::getPayload()
{
    //kDebug(kdeconnect_kded()) << "getPayload";
    return mSocket.staticCast<QIODevice>();
}
