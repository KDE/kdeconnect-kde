/**
 * Copyright 2019 Matthijs Tijink <matthijstijink@gmail.com>
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

#ifndef MULTIPLEXCHANNELSTATE_H
#define MULTIPLEXCHANNELSTATE_H

#include <QObject>
#include <QByteArray>

class ConnectionMultiplexer;
class MultiplexChannel;

/**
 * Represents a single channel in a multiplexed connection
 *
 * @internal
 * @see ConnectionMultiplexer
 */
class MultiplexChannelState : public QObject {
    Q_OBJECT

private:
    MultiplexChannelState();
public:
    ~MultiplexChannelState() = default;

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
