/**
 * SPDX-FileCopyrightText: 2019 Matthijs Tijink <matthijstijink@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef MULTIPLEXCHANNEL_H
#define MULTIPLEXCHANNEL_H

#include <QIODevice>
#include <QSharedPointer>

class ConnectionMultiplexer;
class MultiplexChannelState;

/**
 * Represents a single channel in a multiplexed connection
 *
 * @see ConnectionMultiplexer
 * @see ConnectionMultiplexer::getChannel
 */
class MultiplexChannel : public QIODevice
{
    Q_OBJECT

private:
    /**
     * You cannot construct a MultiplexChannel yourself, use the ConnectionMultiplexer
     */
    MultiplexChannel(QSharedPointer<MultiplexChannelState> state);

public:
    ~MultiplexChannel() override;

    constexpr static int BUFFER_SIZE = 4096;

    bool canReadLine() const override;
    bool atEnd() const override;
    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;

    bool isSequential() const override;

protected:
    qint64 readData(char *data, qint64 maxlen) override;
    qint64 writeData(const char *data, qint64 len) override;

private:
    QSharedPointer<MultiplexChannelState> state;

    /**
     * Disconnects the channel
     */
    void disconnect();

    friend class ConnectionMultiplexer;
};

#endif
