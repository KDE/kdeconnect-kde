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
#include <daemon.h>

UploadJob::UploadJob(const NetworkPacket& networkPacket)
    : KJob()
    , m_networkPacket(networkPacket)
    , m_input(networkPacket.payload())
    , m_socket(nullptr)
{
}

void UploadJob::setSocket(QSslSocket* socket)
{
    m_socket = socket;
    m_socket->setParent(this);
}

void UploadJob::start()
{
     if (!m_input->open(QIODevice::ReadOnly)) {
        qCWarning(KDECONNECT_CORE) << "error when opening the input to upload";
        return; //TODO: Handle error, clean up...
    }
    
    if (!m_socket) {
        qCWarning(KDECONNECT_CORE) << "you must call setSocket() before calling start()";
        return;
    }

    connect(m_input.data(), &QIODevice::aboutToClose, this, &UploadJob::aboutToClose);
    
    bytesUploaded = 0;
    setProcessedAmount(Bytes, bytesUploaded);
    
    connect(m_socket, &QSslSocket::encryptedBytesWritten, this, &UploadJob::encryptedBytesWritten);
        
    uploadNextPacket();
}

void UploadJob::uploadNextPacket()
{
    qint64 bytesAvailable = m_input->bytesAvailable();

    if ( bytesAvailable > 0) {
        qint64 bytesToSend = qMin(m_input->bytesAvailable(), (qint64)4096);
        bytesUploading = m_socket->write(m_input->read(bytesToSend));
    }
    
    if (bytesAvailable <= 0 || bytesUploading < 0) {
        m_input->close();
        disconnect(m_socket, &QSslSocket::encryptedBytesWritten, this, &UploadJob::encryptedBytesWritten);
    }
}

void UploadJob::encryptedBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);
        
    if (m_socket->encryptedBytesToWrite() == 0) {
        bytesUploaded += bytesUploading;
        setProcessedAmount(Bytes, bytesUploaded);
    
        uploadNextPacket();
    }
}

void UploadJob::aboutToClose()
{
    m_socket->disconnectFromHost();
    emitResult();
}

bool UploadJob::stop() {
    m_input->close();
    
    return true;
}

const NetworkPacket UploadJob::getNetworkPacket()
{
    return m_networkPacket;
}
