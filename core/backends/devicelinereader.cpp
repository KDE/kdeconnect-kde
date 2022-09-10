/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 * SPDX-FileCopyrightText: 2014 Alejandro Fiestas Olivares <afiestas@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "devicelinereader.h"

DeviceLineReader::DeviceLineReader(QIODevice *device, QObject *parent)
    : QObject(parent)
    , m_device(device)
{
    connect(m_device, &QIODevice::readyRead, this, &DeviceLineReader::dataReceived);
    connect(m_device, &QIODevice::aboutToClose, this, &DeviceLineReader::disconnected);
}

void DeviceLineReader::dataReceived()
{
    while (m_device->canReadLine()) {
        const QByteArray line = m_device->readLine();
        if (line.length() > 1) {
            m_packets.enqueue(line); // we don't want single \n
        }
    }

    // If we still have things to read from the device, call dataReceived again
    // We do this manually because we do not trust readyRead to be emitted again
    // So we call this method again just in case.
    if (m_device->bytesAvailable() > 0) {
        QMetaObject::invokeMethod(this, "dataReceived", Qt::QueuedConnection);
        return;
    }

    // If we have any packets, tell it to the world.
    if (!m_packets.isEmpty()) {
        Q_EMIT readyRead();
    }
}
