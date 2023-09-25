/**
 * SPDX-FileCopyrightText: 2019 Matthijs Tijink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "connectionmultiplexer.h"

#include "core_debug.h"
#include "multiplexchannel.h"
#include "multiplexchannelstate.h"
#include <QBluetoothSocket>
#include <QBluetoothUuid>
#include <QIODevice>
#include <QtEndian>

/**
 * The default channel uuid. This channel is opened implicitly (without communication).
 */
#define DEFAULT_CHANNEL_UUID "a0d0aaf4-1072-4d81-aa35-902a954b1266"

// Message type constants
constexpr char MESSAGE_PROTOCOL_VERSION = 0;
constexpr char MESSAGE_OPEN_CHANNEL = 1;
constexpr char MESSAGE_CLOSE_CHANNEL = 2;
constexpr char MESSAGE_READ = 3;
constexpr char MESSAGE_WRITE = 4;

ConnectionMultiplexer::ConnectionMultiplexer(QBluetoothSocket *socket, QObject *parent)
    : QObject(parent)
    , mSocket{socket}
    , receivedProtocolVersion{false}
{
    connect(mSocket, &QIODevice::readyRead, this, &ConnectionMultiplexer::readyRead);
    connect(mSocket, &QIODevice::aboutToClose, this, &ConnectionMultiplexer::disconnected);
    connect(mSocket, &QBluetoothSocket::disconnected, this, &ConnectionMultiplexer::disconnected);
    connect(mSocket, &QIODevice::bytesWritten, this, &ConnectionMultiplexer::bytesWritten);

    // Send the protocol version
    QByteArray message(23, (char)0);
    message[0] = MESSAGE_PROTOCOL_VERSION;
    qToBigEndian<uint16_t>(4, &message.data()[1]);
    // Leave UUID empty
    // Only support version 1 (lowest supported = highest supported = 1)
    qToBigEndian<uint16_t>(1, &message.data()[19]);
    qToBigEndian<uint16_t>(1, &message.data()[21]);

    socket->write(message);

    // Send the protocol version message (queued)
    QMetaObject::invokeMethod(this, &ConnectionMultiplexer::bytesWritten, Qt::QueuedConnection);

    // Always open the default channel
    addChannel(QBluetoothUuid{QStringLiteral(DEFAULT_CHANNEL_UUID)});

    // Immediately check if we can read stuff ("readyRead" may not be called in that case)
    if (mSocket->bytesAvailable()) {
        // But invoke it queued
        QMetaObject::invokeMethod(this, &ConnectionMultiplexer::readyRead, Qt::QueuedConnection);
    }
}

ConnectionMultiplexer::~ConnectionMultiplexer()
{
    // Always make sure we close the connection
    close();
}

void ConnectionMultiplexer::readyRead()
{
    // Continue parsing messages until we need more data for another message
    while (tryParseMessage()) { }
}

void ConnectionMultiplexer::disconnected()
{
    // In case we get disconnected, remove all channels
    for (auto &&channel : channels) {
        disconnect(channel.data(), nullptr, this, nullptr);
        channel->disconnected();
    }
    channels.clear();
    for (auto channel : unrequested_channels) {
        delete channel;
    }
    unrequested_channels.clear();
}

void ConnectionMultiplexer::close()
{
    // In case we want to close the connection, remove all channels
    for (auto &&channel : channels) {
        disconnect(channel.data(), nullptr, this, nullptr);
        channel->disconnected();
    }
    channels.clear();
    for (auto channel : unrequested_channels) {
        delete channel;
    }
    unrequested_channels.clear();

    mSocket->close();
}

bool ConnectionMultiplexer::isOpen() const
{
    return mSocket->isOpen();
}

bool ConnectionMultiplexer::tryParseMessage()
{
    mSocket->startTransaction();

    // The message header is 19 bytes long
    QByteArray header = mSocket->read(19);
    if (header.size() != 19) {
        mSocket->rollbackTransaction();
        return false;
    }

    /**
     * Parse the header:
     *  - message type (1 byte)
     *  - message length (2 bytes, Big-Endian), excludes header size
     *  - channel uuid (16 bytes, Big-Endian)
     */
    char message_type = header[0];
    uint16_t message_length = qFromBigEndian<uint16_t>(&header.data()[1]);

    quint128 message_uuid_raw;
#ifndef QT_SUPPORTS_INT128
    for (int i = 0; i < 16; ++i) {
        message_uuid_raw.data[i] = header[3 + i];
    }
#else
    message_uuid_raw = qFromBigEndian<quint128>(&header.data()[3]);
#endif
    QBluetoothUuid message_uuid = QBluetoothUuid(message_uuid_raw);

    // Check if we have the full message including its data
    QByteArray data = mSocket->read(message_length);
    if (data.size() != message_length) {
        mSocket->rollbackTransaction();
        return false;
    }

    Q_ASSERT(receivedProtocolVersion || message_type == MESSAGE_PROTOCOL_VERSION);

    // Parse the different message types
    if (message_type == MESSAGE_OPEN_CHANNEL) {
        // The other endpoint requested us to open a channel
        Q_ASSERT(message_length == 0);

        addChannel(message_uuid);
    } else if (message_type == MESSAGE_READ) {
        // The other endpoint has read some data and requests more data
        Q_ASSERT(message_length == 2);
        // Read the number of bytes requested (2 bytes, Big-Endian)
        uint16_t additional_read = qFromBigEndian<uint16_t>(data.data());
        Q_ASSERT(additional_read > 0);

        // Check if we haven't closed the channel in the meanwhile
        //    (note: different from the user's endpoint of a closed channel, since we might have outstanding buffers)
        auto iter = channels.find(message_uuid);
        if (iter != channels.end() && (*iter)->connected) {
            auto channel = *iter;

            // We have "additional_read" more bytes we can safely write in this channel
            channel->freeWriteAmount += additional_read;
            mSocket->commitTransaction();
            // We might still have some data in the write buffer
            Q_EMIT channel->writeAvailable();
            return true;
        }
    } else if (message_type == MESSAGE_WRITE) {
        // The other endpoint has written data into a channel (because we requested it)
        Q_ASSERT(message_length > 0);

        // Check if we haven't closed the channel in the meanwhile
        //    (note: different from the user's endpoint of a closed channel, since we might have outstanding buffers)
        auto iter = channels.find(message_uuid);
        if (iter != channels.end() && (*iter)->connected) {
            auto channel = *iter;

            Q_ASSERT(channel->requestedReadAmount >= message_length);

            // We received some data, so update the buffer and the amount of outstanding read requests
            channel->requestedReadAmount -= message_length;
            channel->read_buffer.append(std::move(data));

            mSocket->commitTransaction();
            // Indicate that the channel can read some bytes
            Q_EMIT channel->readyRead();
            return true;
        }
    } else if (message_type == MESSAGE_CLOSE_CHANNEL) {
        // The other endpoint wants to close a channel
        Q_ASSERT(message_length == 0);

        // Check if we haven't closed the channel in the meanwhile
        //    (note: different from the user's endpoint of a closed channel, since we might have outstanding buffers)
        auto iter = channels.find(message_uuid);
        if (iter != channels.end()) {
            auto channel = *iter;

            // We don't want signals anymore, since the channel is closed
            disconnect(channel.data(), nullptr, this, nullptr);
            removeChannel(message_uuid);
        }
    } else if (message_type == MESSAGE_PROTOCOL_VERSION) {
        // Checks for protocol compatibility
        Q_ASSERT(message_length >= 4);
        // Read the lowest & highest version supported (each 2 bytes, Big-Endian)
        uint16_t lowest_version = qFromBigEndian<uint16_t>(&data.data()[0]);
        uint16_t highest_version = qFromBigEndian<uint16_t>(&data.data()[2]);

        Q_ASSERT(lowest_version == 1);
        Q_ASSERT(highest_version >= 1);
        receivedProtocolVersion = true;
    } else {
        // Other message types are not supported
        Q_ASSERT(false);
    }

    mSocket->commitTransaction();
    return true;
}

QBluetoothUuid ConnectionMultiplexer::newChannel()
{
    // Create a random uuid
    QBluetoothUuid new_id(QUuid::createUuid());

    // Open the channel on the other endpoint
    QByteArray message(3, (char)0);
    message[0] = MESSAGE_OPEN_CHANNEL;
    qToBigEndian<uint16_t>(0, &message.data()[1]);

#if QT_VERSION_MAJOR == 5
    quint128 id_raw = new_id.toUInt128();
    message.append((const char *)id_raw.data, 16);
#else
    const auto channelBytes = new_id.toByteArray();
    message.append(channelBytes.constData(), 16);
#endif
    to_write_bytes.append(message);

    // Add the channel ourselves
    addChannel(new_id);
    // Write the data
    bytesWritten();
    return new_id;
}

void ConnectionMultiplexer::addChannel(QBluetoothUuid new_id)
{
    MultiplexChannelState *channelState = new MultiplexChannelState();
    // Connect all channels queued, so that we have opportunities to combine read/write requests

    Q_ASSERT(unrequested_channels.size() <= 20);

    // Note that none of the channels knows its own uuid, so we have to add it ourselves
    connect(
        channelState,
        &MultiplexChannelState::readAvailable,
        this,
        [new_id, this]() {
            channelCanRead(new_id);
        },
        Qt::QueuedConnection);
    connect(
        channelState,
        &MultiplexChannelState::writeAvailable,
        this,
        [new_id, this]() {
            channelCanWrite(new_id);
        },
        Qt::QueuedConnection);
    connect(
        channelState,
        &MultiplexChannelState::requestClose,
        this,
        [new_id, this]() {
            closeChannel(new_id);
        },
        Qt::QueuedConnection);

    auto channelStatePtr = QSharedPointer<MultiplexChannelState>{channelState};
    channels[new_id] = channelStatePtr;
    unrequested_channels[new_id] = new MultiplexChannel{channelStatePtr};
    // Immediately ask for data in this channel
    Q_EMIT channelStatePtr->readAvailable();
}

std::unique_ptr<MultiplexChannel> ConnectionMultiplexer::getChannel(QBluetoothUuid channelId)
{
    auto iter = unrequested_channels.find(channelId);
    if (iter == unrequested_channels.end()) {
        return nullptr;
    } else if (!(*iter)->isOpen()) {
        // Delete the channel
        delete *iter;
        unrequested_channels.erase(iter);
        // Don't return closed channels
        return nullptr;
    } else {
        auto channel = *iter;
        unrequested_channels.erase(iter);
        return std::unique_ptr<MultiplexChannel>{channel};
    }
}

std::unique_ptr<MultiplexChannel> ConnectionMultiplexer::getDefaultChannel()
{
    return getChannel(QBluetoothUuid{QStringLiteral(DEFAULT_CHANNEL_UUID)});
}

void ConnectionMultiplexer::bytesWritten()
{
    if (to_write_bytes.size() > 0) {
        // If we have stuff to write, try to write it
        auto num_written = mSocket->write(to_write_bytes);
        if (num_written <= 0) {
            // On error: disconnected will be called later
            // On buffer full: will be retried later
            return;
        } else if (num_written == to_write_bytes.size()) {
            to_write_bytes.clear();
        } else {
            to_write_bytes.remove(0, num_written);
            return;
        }
    }
}

void ConnectionMultiplexer::channelCanRead(QBluetoothUuid channelId)
{
    auto iter = channels.find(channelId);
    if (iter == channels.end())
        return;
    auto channel = *iter;

    // Check if we can request more data to read without overflowing the buffer
    if (channel->read_buffer.size() + channel->requestedReadAmount < channel->BUFFER_SIZE) {
        // Request the exact amount to fill up the buffer
        auto read_amount = channel->BUFFER_SIZE - channel->requestedReadAmount - channel->read_buffer.size();
        channel->requestedReadAmount += read_amount;

        // Send a MESSAGE_READ request for more data
        QByteArray message(3, (char)0);
        message[0] = MESSAGE_READ;
        qToBigEndian<uint16_t>(2, &message.data()[1]);
#if QT_VERSION_MAJOR == 5
        quint128 id_raw = channelId.toUInt128();
        message.append((const char *)id_raw.data, 16);
#else
        const auto channelBytes = channelId.toByteArray();
        message.append(channelBytes.constData(), 16);
#endif
        message.append(2, 0);
        qToBigEndian<int16_t>(read_amount, &message.data()[19]);
        to_write_bytes.append(message);
        // Try to send it immediately
        bytesWritten();
    }
}

void ConnectionMultiplexer::channelCanWrite(QBluetoothUuid channelId)
{
    auto iter = channels.find(channelId);
    if (iter == channels.end())
        return;
    auto channel = *iter;

    // Check if we can freely send data and we actually have some data
    if (channel->write_buffer.size() > 0 && channel->freeWriteAmount > 0) {
        // Figure out how much we can send now
        auto amount = qMin((int)channel->write_buffer.size(), channel->freeWriteAmount);
        QByteArray data = channel->write_buffer.left(amount);
        channel->write_buffer.remove(0, amount);
        channel->freeWriteAmount -= amount;

        // Send the data
        QByteArray message(3, (char)0);
        message[0] = MESSAGE_WRITE;
        qToBigEndian<uint16_t>(amount, &message.data()[1]);

#if QT_VERSION_MAJOR == 5
        quint128 id_raw = channelId.toUInt128();
        message.append((const char *)id_raw.data, 16);
#else
        const auto channelBytes = channelId.toByteArray();
        message.append(channelBytes.constData(), 16);
#endif

        message.append(data);
        to_write_bytes.append(message);
        // Try to send it immediately
        bytesWritten();
        // Let the channel's users know that some data has been written
        Q_EMIT channel->bytesWritten(amount);

        // If the user previously asked to close the channel and we finally managed to write the buffer, actually close it
        if (channel->write_buffer.isEmpty() && channel->close_after_write) {
            closeChannel(channelId);
        }
    }
}

void ConnectionMultiplexer::closeChannel(QBluetoothUuid channelId)
{
    auto iter = channels.find(channelId);
    if (iter == channels.end())
        return;
    auto channel = *iter;

    // If the user wants to close a channel, then the user won't be reading from it anymore
    channel->read_buffer.clear();
    channel->close_after_write = true;

    // If there's still stuff to write, don't close it just yet
    if (!channel->write_buffer.isEmpty())
        return;
    channels.erase(iter);
    channel->connected = false;

    // Send the actual close channel message
    QByteArray message(3, (char)0);
    message[0] = MESSAGE_CLOSE_CHANNEL;
    qToBigEndian<uint16_t>(0, &message.data()[1]);

#if QT_VERSION_MAJOR == 5
    quint128 id_raw = channelId.toUInt128();
    message.append((const char *)id_raw.data, 16);
#else
    const auto channelBytes = channelId.toByteArray();
    message.append(channelBytes.constData(), 16);
#endif
    to_write_bytes.append(message);
    // Try to send it immediately
    bytesWritten();
}

void ConnectionMultiplexer::removeChannel(QBluetoothUuid channelId)
{
    auto iter = channels.find(channelId);
    if (iter == channels.end())
        return;
    auto channel = *iter;

    // Remove the channel from the channel list
    channels.erase(iter);
    channel->connected = false;

    Q_EMIT channel->disconnected();
}

#include "moc_connectionmultiplexer.cpp"
