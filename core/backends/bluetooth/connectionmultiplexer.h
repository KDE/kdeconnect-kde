/**
 * SPDX-FileCopyrightText: 2019 Matthijs Tijink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef CONNECTIONMULTIPLEXER_H
#define CONNECTIONMULTIPLEXER_H

#include <QBluetoothAddress>
#include <QByteArray>
#include <QHash>
#include <QObject>
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
class ConnectionMultiplexer : public QObject
{
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

    /**
     * Returns the address of the connected device for display.
     */
    QBluetoothAddress address() const;

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
    QHash<QBluetoothUuid, MultiplexChannel *> unrequested_channels;
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
