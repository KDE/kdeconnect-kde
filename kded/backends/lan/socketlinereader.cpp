/**
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

#include "socketlinereader.h"

#include "../../kdebugnamespace.h"

SocketLineReader::SocketLineReader(QTcpSocket* socket, QObject* parent)
    : QObject(parent)
    , mSocket(socket)
{

    connect(mSocket, SIGNAL(disconnected()),
            this, SIGNAL(disconnected()));

    connect(mSocket, SIGNAL(readyRead()),
            this, SLOT(dataReceived()));

}

void SocketLineReader::dataReceived()
{

    QByteArray data = lastChunk + mSocket->readAll();

    int parsedLength = 0;
    int packageLength = 0;
    Q_FOREACH(char c, data) {
        packageLength++;
        if (c == '\n') {
            QByteArray package = data.mid(parsedLength,packageLength);
            parsedLength += packageLength;
            packageLength = 0;
            if(package.length() > 1) { //Ignore single newlines
                mPackages.enqueue(package);
            }
        }
    }

    lastChunk = data.mid(parsedLength);

    if (mSocket->bytesAvailable() > 0) {

        QMetaObject::invokeMethod(this, "dataReceived", Qt::QueuedConnection);

    } else {

        if (mPackages.length() > 0) {
            Q_EMIT readyRead();
        } else {
            kDebug(kdeconnect_kded()) << "Received incomplete chunk of data, waiting for more";
        }

    }
}
