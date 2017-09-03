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
    , m_input(source)
    , m_server(new Server(this))
    , m_socket(nullptr)
    , m_port(0)
    , m_deviceId(deviceId) // We will use this info if link is on ssl, to send encrypted payload
{
    connect(m_input.data(), &QIODevice::readyRead, this, &UploadJob::startUploading);
    connect(m_input.data(), &QIODevice::aboutToClose, this, &UploadJob::aboutToClose);
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
}

void UploadJob::newConnection()
{
    if (!m_input->open(QIODevice::ReadOnly)) {
        qCWarning(KDECONNECT_CORE) << "error when opening the input to upload";
        return; //TODO: Handle error, clean up...
    }

    Server* server = qobject_cast<Server*>(sender());
    // FIXME : It is called again when payload sending is finished. Unsolved mystery :(
    disconnect(m_server, &QTcpServer::newConnection, this, &UploadJob::newConnection);

    m_socket = server->nextPendingConnection();
    m_socket->setParent(this);
    connect(m_socket, &QSslSocket::disconnected, this, &UploadJob::cleanup);
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketFailed(QAbstractSocket::SocketError)));
    connect(m_socket, SIGNAL(sslErrors(QList<QSslError>)), this, SLOT(sslErrors(QList<QSslError>)));
    connect(m_socket, &QSslSocket::encrypted, this, &UploadJob::startUploading);
//     connect(mSocket, &QAbstractSocket::stateChanged, [](QAbstractSocket::SocketState state){ qDebug() << "statechange" << state; });

    LanLinkProvider::configureSslSocket(m_socket, m_deviceId, true);

    m_socket->startServerEncryption();
}

void UploadJob::startUploading()
{
    while ( m_input->bytesAvailable() > 0 )
    {
        qint64 bytes = qMin(m_input->bytesAvailable(), (qint64)4096);
        int w = m_socket->write(m_input->read(bytes));
        if (w<0) {
            qCWarning(KDECONNECT_CORE) << "error when writing data to upload" << bytes << m_input->bytesAvailable();
            break;
        }
        else
        {
            while ( m_socket->flush() );
        }
    }
    m_input->close();
}

void UploadJob::aboutToClose()
{
//     qDebug() << "closing...";
    m_socket->disconnectFromHost();
}

void UploadJob::cleanup()
{
    m_socket->close();
//     qDebug() << "closed!";
    emitResult();
}

QVariantMap UploadJob::transferInfo()
{
    Q_ASSERT(m_port != 0);
    return {{"port", m_port}};
}

void UploadJob::socketFailed(QAbstractSocket::SocketError error)
{
    qWarning() << "error uploading" << error;
    setError(2);
    emitResult();
    m_socket->close();
}

void UploadJob::sslErrors(const QList<QSslError>& errors)
{
    qWarning() << "ssl errors" << errors;
    setError(1);
    emitResult();
    m_socket->close();
}
