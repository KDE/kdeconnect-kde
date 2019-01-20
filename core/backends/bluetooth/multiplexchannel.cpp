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

#include "multiplexchannel.h"
#include "multiplexchannelstate.h"
#include "core_debug.h"

MultiplexChannel::MultiplexChannel(QSharedPointer<MultiplexChannelState> state) : state{state} {
    QIODevice::open(QIODevice::ReadWrite);

    connect(this, &QIODevice::aboutToClose, state.data(), &MultiplexChannelState::requestClose);
    connect(state.data(), &MultiplexChannelState::readyRead, this, &QIODevice::readyRead);
    connect(state.data(), &MultiplexChannelState::bytesWritten, this, &QIODevice::bytesWritten);
    connect(state.data(), &MultiplexChannelState::disconnected, this, &MultiplexChannel::disconnect);
}

MultiplexChannel::~MultiplexChannel() {}

bool MultiplexChannel::atEnd() const {
    return !isOpen() || (!state->connected && state->read_buffer.isEmpty());
}

void MultiplexChannel::disconnect() {
    state->connected = false;
    setOpenMode(QIODevice::ReadOnly);
    Q_EMIT state->readyRead();
    Q_EMIT state->requestClose();
    if (state->read_buffer.isEmpty()) {
        close();
    }
}

qint64 MultiplexChannel::bytesAvailable() const {
    return state->read_buffer.size() + QIODevice::bytesAvailable();
}

qint64 MultiplexChannel::bytesToWrite() const {
    return state->write_buffer.size() + QIODevice::bytesToWrite();
}

qint64 MultiplexChannel::readData(char* data, qint64 maxlen) {
    if (maxlen <= state->read_buffer.size()) {
        for (int i = 0; i < maxlen; ++i) {
            data[i] = state->read_buffer[i];
        }
        state->read_buffer.remove(0, maxlen);
        Q_EMIT state->readAvailable();
        if (!state->connected && state->read_buffer.isEmpty()) {
            close();
        }
        return maxlen;
    } else if (state->read_buffer.size() > 0) {
        auto num_to_read = state->read_buffer.size();
        for (int i = 0; i < num_to_read; ++i) {
            data[i] = state->read_buffer[i];
        }
        state->read_buffer.remove(0, num_to_read);
        Q_EMIT state->readAvailable();
        if (!state->connected && state->read_buffer.isEmpty()) {
            close();
        }
        return num_to_read;
    } else if (isOpen() && state->connected) {
        if (state->requestedReadAmount < BUFFER_SIZE) {
            Q_EMIT state->readAvailable();
        }
        return 0;
    } else {
        close();
        return -1;
    }
}

qint64 MultiplexChannel::writeData(const char* data, qint64 len) {
    state->write_buffer.append(data, len);
    Q_EMIT state->writeAvailable();
    return len;
}

bool MultiplexChannel::canReadLine() const {
    return isReadable() && (QIODevice::canReadLine() || state->read_buffer.contains('\n'));
}

bool MultiplexChannel::isSequential() const {
    return true;
}
