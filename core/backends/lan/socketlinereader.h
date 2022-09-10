/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef SOCKETLINEREADER_H
#define SOCKETLINEREADER_H

#include <QHostAddress>
#include <QObject>
#include <QQueue>
#include <QSslSocket>

#include <kdeconnectcore_export.h>

/*
 * Encapsulates a QTcpSocket and implements the same methods of its API that are
 * used by LanDeviceLink, but readyRead is emitted only when a newline is found.
 */
class KDECONNECTCORE_EXPORT SocketLineReader : public QObject
{
    Q_OBJECT

public:
    explicit SocketLineReader(QSslSocket *socket, QObject *parent = nullptr);

    bool hasPacketsAvailable() const
    {
        return !m_packets.isEmpty();
    }
    QByteArray readLine()
    {
        return m_packets.dequeue();
    }
    qint64 write(const QByteArray &data)
    {
        return m_socket->write(data);
    }
    QHostAddress peerAddress() const
    {
        return m_socket->peerAddress();
    }
    QSslCertificate peerCertificate() const
    {
        return m_socket->peerCertificate();
    }

    QSslSocket *m_socket;

Q_SIGNALS:
    void readyRead();

private Q_SLOTS:
    void dataReceived();

private:
    QByteArray m_lastChunk;
    QQueue<QByteArray> m_packets;
};

#endif
