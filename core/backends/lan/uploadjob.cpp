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
#include <device.h>
#include <daemon.h>

UploadJob::UploadJob(const QSharedPointer<QIODevice>& source, const QString& deviceId)
    : KJob()
    , m_input(source)
    , m_server(new Server(this))
    , m_socket(nullptr)
    , m_port(0)
    , m_deviceId(deviceId) // We will use this info if link is on ssl, to send encrypted payload
{
    connect(m_input.data(), &QIODevice::aboutToClose, this, &UploadJob::aboutToClose);
    setCapabilities(Killable);
}

void UploadJob::start()
{
    m_port = MIN_PORT;
    while (!m_server->listen(QHostAddress::Any, m_port)) {
        m_port++;
        if (m_port > MAX_PORT) { //No ports available?
            qCWarning(KDECONNECT_CORE) << "Error opening a port in range" << MIN_PORT << "-" << MAX_PORT;
            m_port = 0;
            setError(1);
            setErrorText(i18n("Couldn't find an available port"));
            emitResult();
            return;
        }
    }
    connect(m_server, &QTcpServer::newConnection, this, &UploadJob::newConnection);
   
    Q_EMIT description(this, i18n("Sending file to %1", Daemon::instance()->getDevice(this->m_deviceId)->name()), 
                { i18nc("File transfer origin", "From"), m_input.staticCast<QFile>().data()->fileName() }
    );
}

void UploadJob::newConnection()
{
    if (!m_input->open(QIODevice::ReadOnly)) {
        qCWarning(KDECONNECT_CORE) << "error when opening the input to upload";
        return; //TODO: Handle error, clean up...
    }

    m_socket = m_server->nextPendingConnection();
    m_socket->setParent(this);
    connect(m_socket, &QSslSocket::disconnected, this, &UploadJob::cleanup);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketFailed(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(m_socket, &QSslSocket::encrypted, this, &UploadJob::startUploading);

    LanLinkProvider::configureSslSocket(m_socket, m_deviceId, true);

    m_socket->startServerEncryption();
}

void UploadJob::startUploading()
{
    bytesUploaded = 0;
    setProcessedAmount(Bytes, bytesUploaded);
    setTotalAmount(Bytes, m_input.data()->size());
    
    connect(m_socket, &QSslSocket::encryptedBytesWritten, this, &UploadJob::encryptedBytesWritten);
    
    if (!m_timer.isValid()) {
        m_timer.start();
    }
    
    uploadNextPacket();
}

void UploadJob::uploadNextPacket()
{
    qint64 bytesAvailable = m_input->bytesAvailable();

    if ( bytesAvailable > 0) {
        qint64 bytesToSend = qMin(m_input->bytesAvailable(), (qint64)4096);
        bytesUploading = m_socket->write(m_input->read(bytesToSend));

        if (bytesUploading < 0) {
            qCWarning(KDECONNECT_CORE) << "error when writing data to upload" << bytesToSend << m_input->bytesAvailable();
        }
    }
    
    if (bytesAvailable <= 0 || bytesUploading < 0) {
        m_input->close();
        disconnect(m_socket, &QSslSocket::encryptedBytesWritten, this, &UploadJob::encryptedBytesWritten);
    }
}

void UploadJob::encryptedBytesWritten(qint64 bytes)
{
    Q_UNUSED(bytes);
    
    bytesUploaded += bytesUploading;
    
    if (m_socket->encryptedBytesToWrite() == 0) {
        setProcessedAmount(Bytes, bytesUploaded);
    
        const auto elapsed = m_timer.elapsed();
        if (elapsed > 0) {
            emitSpeed((1000 * bytesUploaded) / elapsed);
        }
    
        uploadNextPacket();
    }
}

void UploadJob::aboutToClose()
{
    qWarning() << "aboutToClose()";
    m_socket->disconnectFromHost();
}

void UploadJob::cleanup()
{
    qWarning() << "cleanup()";
    m_socket->close();
    emitResult();
}

QVariantMap UploadJob::transferInfo()
{
    Q_ASSERT(m_port != 0);
    return {{"port", m_port}};
}

void UploadJob::socketFailed(QAbstractSocket::SocketError error)
{
    qWarning() << "socketFailed() " << error;
    m_socket->close();
    setError(2);
    emitResult();
}

void UploadJob::sslErrors(const QList<QSslError>& errors)
{
    qWarning() << "sslErrors() " << errors;
    setError(1);
    emitResult();
    m_socket->close();
}

bool UploadJob::doKill()
{    
    if (m_input) {
        m_input->close();
    }
    return true;
}
