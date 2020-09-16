/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2014 Alejandro Fiestas Olivares <afiestas@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "socketlinereader.h"

SocketLineReader::SocketLineReader(QSslSocket* socket, QObject* parent)
    : QObject(parent)
    , m_socket(socket)
{
    connect(m_socket, &QIODevice::readyRead,
            this, &SocketLineReader::dataReceived);
}

void SocketLineReader::dataReceived()
{
    while (m_socket->canReadLine()) {
        const QByteArray line = m_socket->readLine();
        if (line.length() > 1) { //we don't want a single \n
            m_packets.enqueue(line);
        }
    }

    //If we have any packets, tell it to the world.
    if (!m_packets.isEmpty()) {
        Q_EMIT readyRead();
    }
}
