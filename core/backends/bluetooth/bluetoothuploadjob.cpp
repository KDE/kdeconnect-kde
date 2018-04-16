/*
 * Copyright 2016 Saikrishna Arcot <saiarcot895@gmail.com>
 * Copyright 2018 Matthijs TIjink <matthijstijink@gmail.com>
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

#include "bluetoothuploadjob.h"

#include <QBluetoothSocket>
#include "core_debug.h"
#include <QCoreApplication>

BluetoothUploadJob::BluetoothUploadJob(const QSharedPointer<QIODevice>& data, const QBluetoothAddress& remoteAddress, QObject* parent)
    : QObject(parent)
    , mData(data)
    , mRemoteAddress(remoteAddress)
    , mTransferUuid(QBluetoothUuid::createUuid())
    , mServer(new QBluetoothServer(QBluetoothServiceInfo::RfcommProtocol, this))
{
    mServer->setSecurityFlags(QBluetooth::Encryption | QBluetooth::Secure);
}

QVariantMap BluetoothUploadJob::transferInfo() const
{
    QVariantMap ret;
    ret["uuid"] = mTransferUuid.toString().mid(1, 36);
    return ret;
}

void BluetoothUploadJob::start()
{
    connect(mServer, &QBluetoothServer::newConnection, this, &BluetoothUploadJob::newConnection);
    mServiceInfo = mServer->listen(mTransferUuid, "KDE Connect Transfer Job");
    Q_ASSERT(mServiceInfo.isValid());
}

void BluetoothUploadJob::newConnection()
{
    m_socket = mServer->nextPendingConnection();
    Q_ASSERT(m_socket);
    m_socket->setParent(this);
    connect(m_socket, &QBluetoothSocket::disconnected, this, &BluetoothUploadJob::deleteLater);

    if (m_socket->peerAddress() != mRemoteAddress) {
        m_socket->close();
    } else {
        mServer->close();
        disconnect(mServer, &QBluetoothServer::newConnection, this, &BluetoothUploadJob::newConnection);
        mServiceInfo.unregisterService();

        if (!mData->open(QIODevice::ReadOnly)) {
            qCWarning(KDECONNECT_CORE) << "error when opening the input to upload";
            m_socket->close();
            deleteLater();
            return;
        }
    }

    connect(m_socket, &QBluetoothSocket::bytesWritten, this, &BluetoothUploadJob::writeSome);
    connect(m_socket, &QBluetoothSocket::disconnected, this, &BluetoothUploadJob::closeConnection);
    writeSome();
}

void BluetoothUploadJob::writeSome() {
    Q_ASSERT(m_socket);

    bool errorOccurred = false;
    while (m_socket->bytesToWrite() == 0 && mData->bytesAvailable() && m_socket->isWritable()) {
        qint64 bytes = qMin<qint64>(mData->bytesAvailable(), 4096);
        int bytesWritten = m_socket->write(mData->read(bytes));

        if (bytesWritten < 0) {
            qCWarning(KDECONNECT_CORE()) << "error when writing data to bluetooth upload" << bytes << mData->bytesAvailable();
            errorOccurred = true;
            break;
        }
    }

    if (mData->atEnd() || errorOccurred) {
        disconnect(m_socket, &QBluetoothSocket::bytesWritten, this, &BluetoothUploadJob::writeSome);
        mData->close();

        connect(m_socket, &QBluetoothSocket::bytesWritten, this, &BluetoothUploadJob::finishWrites);
        finishWrites();
    }
}

void BluetoothUploadJob::finishWrites() {
    Q_ASSERT(m_socket);
    if (m_socket->bytesToWrite() == 0) {
        closeConnection();
    }
}

void BluetoothUploadJob::closeConnection() {
    mData->close();
    deleteLater();
}
