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
class MultiplexChannel : public QIODevice {
    Q_OBJECT

private:
    /**
     * You cannot construct a MultiplexChannel yourself, use the ConnectionMultiplexer
     */
    MultiplexChannel(QSharedPointer<MultiplexChannelState> state);
public:
    ~MultiplexChannel();

    constexpr static int BUFFER_SIZE = 4096;

    bool canReadLine() const override;
    bool atEnd() const override;
    qint64 bytesAvailable() const override;
    qint64 bytesToWrite() const override;

    bool isSequential() const override;

protected:
    qint64 readData(char * data, qint64 maxlen) override;
    qint64 writeData(const char * data, qint64 len) override;

private:
    QSharedPointer<MultiplexChannelState> state;

    /**
     * Disconnects the channel
     */
    void disconnect();

    friend class ConnectionMultiplexer;
};

#endif
