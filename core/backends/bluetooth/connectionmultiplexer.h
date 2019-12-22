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

#ifndef CONNECTIONMULTIPLEXER_H
#define CONNECTIONMULTIPLEXER_H

#include <QObject>
#include <QByteArray>
#include <QHash>
#include <QSharedPointer>
#include <memory>

class QBluetoothUuid;
class MultiplexChannel;
class MultiplexChannelState;
class QBluetoothSocket;

/**
 * An utility class to split a single (bluetooth) connection to multiple independent channels.
 * By default (and without needing any communication with the other endpoint), a single default channel is open.
 *
 * Destroying/closing this object will automatically close all channels.
 */
class ConnectionMultiplexer : public QObject {
    Q_OBJECT
public:
    ConnectionMultiplexer(QBluetoothSocket *socket, QObject *parent = nullptr);
    ~ConnectionMultiplexer();

    /**
     * Open a new channel within this connection.
     *
     * @return The uuid to refer to this channel.
     * @see getChannel()
     */
    QBluetoothUuid newChannel();
    /**
     * Get the channel device for the specified channel uuid.
     * If the channel does not exist, this will return a null pointer.
     *
     * A channel is guaranteed to exist until the first call to get it.
     * @param channelId The channel uuid
     * @return A shared pointer to the channel object
     * @see getDefaultChannel()
     */
    std::unique_ptr<MultiplexChannel> getChannel(QBluetoothUuid channelId);
    /**
     * Get the default channel.
     *
     * @see getChannel()
     */
    std::unique_ptr<MultiplexChannel> getDefaultChannel();

    /**
     * Close all channels and the underlying connection.
     */
    void close();

    /**
     * Check if the underlying connection is still open.
     * @return True if the connection is open
     * @see close()
     */
    bool isOpen() const;

private:
    /**
     * The underlying connection
     */
    QBluetoothSocket *mSocket;
    /**
     * The buffer of to-be-written bytes
     */
    QByteArray to_write_bytes;
    /**
     * The channels not requested by the user yet
     */
    QHash<QBluetoothUuid, MultiplexChannel*> unrequested_channels;
    /**
     * All channels currently open
     */
    QHash<QBluetoothUuid, QSharedPointer<MultiplexChannelState>> channels;
    /**
     * True once the other side has sent its protocol version
     */
    bool receivedProtocolVersion;

    /**
     * Slot for connection reading
     */
    void readyRead();
    /**
     * Slot for disconnection
     */
    void disconnected();
    /**
     * Slot for progress in writing data/new data available to be written
     */
    void bytesWritten();
    /**
     * Tries to parse a single connection message.
     *
     * @return True if a message was parsed successfully.
     */
    bool tryParseMessage();
    /**
     * Add a new channel. Assumes that the communication about this channel is done
     * (i.e. the other endpoint also knows this channel exists).
     *
     * @param new_id The channel uuid
     */
    void addChannel(QBluetoothUuid new_id);

    /**
     * Slot for closing a channel
     */
    void closeChannel(QBluetoothUuid channelId);
    /**
     * Slot for writing a channel's data to the other endpoint
     */
    void channelCanWrite(QBluetoothUuid channelId);
    /**
     * Slot for indicating that a channel can receive more data
     */
    void channelCanRead(QBluetoothUuid channelId);
    /**
     * Slot for removing a channel from tracking
     */
    void removeChannel(QBluetoothUuid channelId);
};

#endif
