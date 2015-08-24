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

#include <qalgorithms.h>
#include <QtGlobal>
#include <kdeconnectconfig.h>

#include "core_debug.h"

UploadJob::UploadJob(const QSharedPointer<QIODevice>& source, const QVariantMap& transferInfo): KJob()
{
    mInput = source;
    mServer = new Server(this);
    mSocket = 0;
    mPort = 0;

    // We will use this info if link is on ssl, to send encrypted payload
    this->mTransferInfo = transferInfo;

    connect(mInput.data(), SIGNAL(readyRead()), this, SLOT(readyRead()));
    connect(mInput.data(), SIGNAL(aboutToClose()), this, SLOT(aboutToClose()));
}

void UploadJob::start()
{
    mPort = 1739;
    while (!mServer->listen(QHostAddress::Any, mPort)) {
        mPort++;
        if (mPort > 1764) { //No ports available?
            qWarning(KDECONNECT_CORE) << "Error opening a port in range 1739-1764 for file transfer";
            mPort = 0;
            return;
        }
    }
    connect(mServer, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

void UploadJob::newConnection()
{
    if (!mInput->open(QIODevice::ReadOnly)) {
        qWarning() << "error when opening the input to upload";
        return; //TODO: Handle error, clean up...
    }

    Server* server = qobject_cast<Server*>(sender());
    // FIXME : It is called again when payload sending is finished. Unsolved mystery :(
    disconnect(mServer, SIGNAL(newConnection()), this, SLOT(newConnection()));

    mSocket = server->nextPendingConnection();
    connect(mSocket, SIGNAL(disconnected()), mSocket, SLOT(deleteLater()));

    if (mTransferInfo.value("useSsl", false).toBool()) {
        mSocket->setLocalCertificate(KdeConnectConfig::instance()->certificate());
        mSocket->setPrivateKey(KdeConnectConfig::instance()->privateKeyPath());
        mSocket->setProtocol(QSsl::TlsV1_2);
        mSocket->setPeerVerifyName(mTransferInfo.value("deviceId").toString());
        mSocket->addCaCertificate(QSslCertificate(KdeConnectConfig::instance()->getTrustedDevice(mTransferInfo.value("deviceId").toString()).certificate.toLatin1()));
        mSocket->startServerEncryption();
        mSocket->waitForEncrypted();
    }

    readyRead();
}

void UploadJob::readyRead()
{
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

QVariantMap UploadJob::transferInfo()
{
    Q_ASSERT(mPort != 0);

    QVariantMap ret;
    ret["port"] = mPort;
    return ret;
}

