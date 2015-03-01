/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 * Copyright 2014 Alejandro Fiestas Olivares <afiestas@kde.org>
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

#include "devicelinereader.h"

DeviceLineReader::DeviceLineReader(QIODevice* device, QObject* parent)
    : QObject(parent)
    , mDevice(device)
{
    connect(mDevice, SIGNAL(readyRead()),
            this, SLOT(dataReceived()));
    connect(mDevice, SIGNAL(disconnected()),
            this, SIGNAL(disconnected()));
}

void DeviceLineReader::dataReceived()
{
    while(mDevice->canReadLine()) {
        const QByteArray line = mDevice->readLine();
        if (line.length() > 1) {
            mPackages.enqueue(line);//we don't want single \n
        }
    }

    //If we still have things to read from the device, call dataReceived again
    //We do this manually because we do not trust readyRead to be emitted again
    //So we call this method again just in case.
    if (mDevice->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "dataReceived", Qt::QueuedConnection);
        return;
    }

    //If we have any packages, tell it to the world.
    if (!mPackages.isEmpty()) {
        Q_EMIT readyRead();
    }
}
