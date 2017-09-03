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

DownloadJob::DownloadJob(const QHostAddress& address, const QVariantMap& transferInfo)
    : KJob()
    , m_address(address)
    , m_port(transferInfo[QStringLiteral("port")].toInt())
    , m_socket(new QSslSocket)
    , m_buffer(new QBuffer)
{
    LanLinkProvider::configureSslSocket(m_socket.data(), transferInfo.value(QStringLiteral("deviceId")).toString(), true);

    connect(m_socket.data(), SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketFailed(QAbstractSocket::SocketError)));
//     connect(mSocket.data(), &QAbstractSocket::stateChanged, [](QAbstractSocket::SocketState state){ qDebug() << "statechange" << state; });
}

DownloadJob::~DownloadJob()
{

}

void DownloadJob::start()
{
    //TODO: Timeout?
    // Cannot use read only, might be due to ssl handshake, getting QIODevice::ReadOnly error and no connection
    m_socket->connectToHostEncrypted(m_address.toString(), m_port, QIODevice::ReadWrite);

    bool b = m_buffer->open(QBuffer::ReadWrite);
    Q_ASSERT(b);
}

void DownloadJob::socketFailed(QAbstractSocket::SocketError error)
{
    if (error != QAbstractSocket::RemoteHostClosedError) { //remote host closes when finishes
        qWarning(KDECONNECT_CORE) << "error..." << m_socket->errorString();
        setError(error + 1);
        setErrorText(m_socket->errorString());
    } else {
        auto ba = m_socket->readAll();
        m_buffer->write(ba);
        m_buffer->seek(0);
    }
    emitResult();
}

QSharedPointer<QIODevice> DownloadJob::getPayload()
{
    return m_buffer.staticCast<QIODevice>();
}
