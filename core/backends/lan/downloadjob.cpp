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
{
    LanLinkProvider::configureSslSocket(m_socket.data(), transferInfo.value(QStringLiteral("deviceId")).toString(), true);

    connect(m_socket.data(), SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketFailed(QAbstractSocket::SocketError)));
    connect(m_socket.data(), &QAbstractSocket::connected, this, &DownloadJob::socketConnected);
    // emit readChannelFinished when the socket gets disconnected. This seems to be a bug in upstream QSslSocket.
    // Needs investigation and upstreaming of the fix. QTBUG-62257
    connect(m_socket.data(), &QAbstractSocket::disconnected, m_socket.data(), &QAbstractSocket::readChannelFinished);
}

DownloadJob::~DownloadJob()
{

}

void DownloadJob::start()
{
    //TODO: Timeout?
    // Cannot use read only, might be due to ssl handshake, getting QIODevice::ReadOnly error and no connection
    m_socket->connectToHostEncrypted(m_address.toString(), m_port, QIODevice::ReadWrite);
}

void DownloadJob::socketFailed(QAbstractSocket::SocketError error)
{
    qWarning() << error << m_socket->errorString();
    setError(error + 1);
    setErrorText(m_socket->errorString());
    emitResult();
}

QSharedPointer<QIODevice> DownloadJob::getPayload()
{
    return m_socket.staticCast<QIODevice>();
}

void DownloadJob::socketConnected()
{
    emitResult();
}
