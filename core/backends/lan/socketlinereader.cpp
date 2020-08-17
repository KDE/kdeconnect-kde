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

    //If we still have things to read from the socket, call dataReceived again
    //We do this manually because we do not trust readyRead to be emitted again
    //So we call this method again just in case.
    if (m_socket->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "dataReceived", Qt::QueuedConnection);
        return;
    }

    //If we have any packets, tell it to the world.
    if (!m_packets.isEmpty()) {
        Q_EMIT readyRead();
    }
}
