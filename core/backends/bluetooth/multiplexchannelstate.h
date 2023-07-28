/**
 * SPDX-FileCopyrightText: 2019 Matthijs Tijink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef MULTIPLEXCHANNELSTATE_H
#define MULTIPLEXCHANNELSTATE_H

#include <QByteArray>
#include <QObject>

class ConnectionMultiplexer;
class MultiplexChannel;

/**
 * Represents a single channel in a multiplexed connection
 *
 * @internal
 * @see ConnectionMultiplexer
 */
class MultiplexChannelState : public QObject
{
    Q_OBJECT

private:
    MultiplexChannelState();

public:
    constexpr static int BUFFER_SIZE = 4096;

private:
    /**
     * The read buffer (already read from underlying connection but not read by the user of the channel)
     */
    QByteArray read_buffer;
    /**
     * The write buffer (already written by the user of the channel, but not to the underlying connection yet)
     */
    QByteArray write_buffer;
    /**
     * The amount of bytes requested to the other endpoint
     */
    int requestedReadAmount;
    /**
     * The amount of bytes the other endpoint requested
     */
    int freeWriteAmount;
    /**
     * True if the channel is still connected in the underlying connection
     */
    bool connected;
    /**
     * True if the channel needs to be closed after writing the buffer
     */
    bool close_after_write;
    friend class ConnectionMultiplexer;
    friend class MultiplexChannel;

Q_SIGNALS:
    /**
     * Emitted if there are new bytes available to be written
     */
    void writeAvailable();
    /**
     * Emitted if the channel has buffer room for more bytes to be read
     */
    void readAvailable();
    /**
     * Emitted if the channel bytes ready for reading
     */
    void readyRead();
    /**
     * Emitted if some bytes of the channel have been written
     */
    void bytesWritten(qint64 amount);
    /**
     * Emitted if the user requested to close the channel
     */
    void requestClose();
    /**
     * Only fired on disconnections
     */
    void disconnected();
};

#endif
