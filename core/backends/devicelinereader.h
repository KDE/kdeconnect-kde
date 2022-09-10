/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DEVICELINEREADER_H
#define DEVICELINEREADER_H

#include <QIODevice>
#include <QObject>
#include <QQueue>
#include <QString>

/*
 * Encapsulates a QIODevice and implements the same methods of its API that are
 * used by LanDeviceLink and BluetoothDeviceLink, but readyRead is emitted only
 * when a newline is found.
 */
class DeviceLineReader : public QObject
{
    Q_OBJECT

public:
    DeviceLineReader(QIODevice *device, QObject *parent = 0);

    QByteArray readLine()
    {
        return m_packets.dequeue();
    }
    qint64 write(const QByteArray &data)
    {
        return m_device->write(data);
    }
    qint64 bytesAvailable() const
    {
        return m_packets.size();
    }

Q_SIGNALS:
    void readyRead();
    void disconnected();

private Q_SLOTS:
    void dataReceived();

private:
    QByteArray m_lastChunk;
    QIODevice *m_device;
    QQueue<QByteArray> m_packets;
};

#endif
