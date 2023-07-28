/**
 * SPDX-FileCopyrightText: 2015 Vineet Garg <grg.vineet@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECT_SERVER_H
#define KDECONNECT_SERVER_H

#include <QSslSocket>
#include <QTcpServer>

#include "kdeconnectcore_export.h"

// This class overrides QTcpServer to bind QSslSocket to native socket descriptor instead of QTcpSocket
class KDECONNECTCORE_EXPORT Server : public QTcpServer
{
    Q_OBJECT

public:
    Server(QObject *parent = nullptr);

    QSslSocket *nextPendingConnection() override;
    void close();

Q_SIGNALS:
    void closed();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    void errorFound(QAbstractSocket::SocketError socketError);
};

#endif // KDECONNECT_SERVER_H
