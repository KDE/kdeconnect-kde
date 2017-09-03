/**
 * Copyright 2015 Vineet Garg <grg.vineet@gmail.com>
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

#include "server.h"

#include "kdeconnectconfig.h"
#include "lanlinkprovider.h"

#include <QSslKey>
#include <QSslSocket>
#include <QSslError>

Server::Server(QObject * parent)
    :QTcpServer(parent)
{
    connect(this, &QTcpServer::acceptError, this, &Server::errorFound);
}

void Server::incomingConnection(qintptr socketDescriptor) {
    QSslSocket* serverSocket = new QSslSocket(parent());
    if (serverSocket->setSocketDescriptor(socketDescriptor)) {
        pendingConnections.append(serverSocket);
        Q_EMIT newConnection();
    } else {
        qWarning() << "setSocketDescriptor failed " + serverSocket->errorString();
        delete serverSocket;
    }
}

QSslSocket* Server::nextPendingConnection() {
    if (pendingConnections.isEmpty()) {
        return Q_NULLPTR;
    } else {
        return pendingConnections.takeFirst();
    }
}

bool Server::hasPendingConnections() const {
    return !pendingConnections.isEmpty();
}

void Server::errorFound(QAbstractSocket::SocketError socketError)
{
    qDebug() << "error:" << socketError;
}
