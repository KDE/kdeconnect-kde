# Bluetooth multiplexing protocol
For bluetooth in KDE Connect, we set up only a single connection between two KDE Connect clients (one as bluetooth server, one as client). This is in contrast to the LAN/TCP backend, where payloads are transferred over separate network connections.

## Why do we do this?
Bluetooth has several issues, compared to TCP. Among these is that the amount of connections between two devices is limited, and that a lot of devices only support a single "server" socket. Also, not all devices can operate in a server mode, only the "master" bluetooth device can.

However, just simply putting the payload directly in the existing connection also has problems. For example, streams without a known size are impossible to support in this scheme. Also, we'd prefer KDE Connect to keep working while a large file is transferring.

## What is the chosen solution?
To solve all this, we implemented a multiplexing protocol. This way, the bluetooth connection is divided up into any number of channels, which can send and receive data independently from each other.

The protocol is agnostic for server/client roles, requires no start-up communication and does not depend on bluetooth peculiarities (so could even by used in e.g. LAN).

# Protocol description
*Protocol version 1*
Every channel on the connection is identified by a randomly chosen UUID. To communicate over the channel, or to communicate *about* a channel, messages are send. The following section explains the general message format, and the sections thereafter describe each type of message.

The first message sent must be the `MESSAGE_PROTOCOL_VERSION` message. After that, other messages can be send in any order, but messages about unknown channels should be ignored.

The multiplexed connection starts with a single default/main channel with UUID **`a0d0aaf4-1072-4d81-aa35-902a954b1266`**. This channel works like any other channel, but requires no communication to set up.

## General message format
Every message send between the two endpoints has the following format.

```
| Message type | Message length       | Channel UUID          | Message data   |
| 1 byte       | 2 bytes (Big-Endian) | 16 bytes (Big-Endian) | "length" bytes |
| ------------------------- Header -------------------------- |                |
```

Where the message type can be one of the following.

| Message type               | Value |
|----------------------------|-------|
| `MESSAGE_PROTOCOL_VERSION` | 0     |
| `MESSAGE_OPEN_CHANNEL`     | 1     |
| `MESSAGE_CLOSE_CHANNEL`    | 2     |
| `MESSAGE_READ`             | 3     |
| `MESSAGE_WRITE`            | 4     |


## MESSAGE_PROTOCOL_VERSION
This message should be the first message send, and never at a later time. Its format is as follows:

```
| MESSAGE_PROTOCOL_VERSION header | Lowest version supported | Highest version supported | Other data           |
| 19 bytes (UUID ignored)         | 2 bytes (Big-Endian)     | 2 bytes (Big-Endian)      | Remaining data bytes |
```

This message should be the first message to send. Use the maximum version supported by both endpoints (if any), or otherwise close the connection. The other data field is not used (and should be empty for protocol version 1), but it implies that message lengths of more than 4 need to be supported for future compatibility.

Currently, no client will send this message with a version other than 1, but you *must* accept and check it, for forward compatibility.

## MESSAGE_OPEN_CHANNEL
Before sending other messages about a channel, first send a `MESSAGE_OPEN_CHANNEL` message, so the other endpoint knows the channel exists. A channel UUID should be chosen randomly and not reused.

This message always has length 0.

## MESSAGE_CLOSE_CHANNEL
To close a channel, send a `MESSAGE_CLOSE_CHANNEL` message. In your implementation, take care that your code gets to see the channel if it's quickly opened, little data is written, and then gets closed again.

This message always has length 0.

## MESSAGE_READ
To prevent getting too much data on a channel unable to cope with that data, each channel needs to indicate that it wants to receive more data. To do so, send a `MESSAGE_READ` message as follows:

```
| MESSAGE_READ header | Amount of additional data requested |
| 19 bytes            | 2 bytes (Big-Endian)                |
```

Note that the amount of data you request is in *addition* to the amounts you requested before.

## MESSAGE_WRITE
To write data in a channel, you must first wait until the other endpoint indicates that it wants some data (with `MESSAGE_READ` messages). Once you have this indication, you must not write more data than has been requested (writing less is allowed). The `MESSAGE_WRITE` message has the following format:

```
| MESSAGE_WRITE header | Data to write in channel |
| 19 bytes             |                          |
```
