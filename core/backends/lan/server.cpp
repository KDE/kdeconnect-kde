/**
 * SPDX-FileCopyrightText: 2015 Vineet Garg <grg.vineet@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "server.h"

#include "kdeconnectconfig.h"
#include "lanlinkprovider.h"

#include <QSslError>
#include <QSslKey>
#include <QSslSocket>

Server::Server(QObject *parent)
    : QTcpServer(parent)
{
    connect(this, &QTcpServer::acceptError, this, &Server::errorFound);
}

void Server::incomingConnection(qintptr socketDescriptor)
{
    QSslSocket *serverSocket = new QSslSocket(parent());
    if (serverSocket->setSocketDescriptor(socketDescriptor)) {
        QObject::connect(this, &Server::closed, serverSocket, &QSslSocket::abort);
        addPendingConnection(serverSocket);
    } else {
        qWarning() << "setSocketDescriptor failed" << serverSocket->errorString();
        delete serverSocket;
    }
}

QSslSocket *Server::nextPendingConnection()
{
    return qobject_cast<QSslSocket *>(QTcpServer::nextPendingConnection());
}

void Server::errorFound(QAbstractSocket::SocketError socketError)
{
    qDebug() << "error:" << socketError;
}

void Server::close()
{
    QTcpServer::close();
    Q_EMIT closed();
}

#include "moc_server.cpp"
