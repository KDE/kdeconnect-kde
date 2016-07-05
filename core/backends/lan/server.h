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

#ifndef KDECONNECT_SERVER_H
#define KDECONNECT_SERVER_H

#include <QTcpServer>
#include <QSslSocket>

#include "kdeconnectcore_export.h"

// This class overrides QTcpServer to bind QSslSocket to native socket descriptor instead of QTcpSocket
class KDECONNECTCORE_EXPORT Server
    : public QTcpServer
{

    Q_OBJECT
private:
    QList<QSslSocket*> pendingConnections;

public:
    Server(QObject* parent = 0);
    ~Server() override = default;

    QSslSocket* nextPendingConnection() override;
    bool hasPendingConnections() const override;

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    void errorFound(QAbstractSocket::SocketError socketError);
};

#endif //KDECONNECT_SERVER_H
