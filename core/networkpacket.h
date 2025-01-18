/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef NETWORKPACKET_H
#define NETWORKPACKET_H

#include "networkpackettypes.h"

#include <QIODevice>
#include <QObject>
#include <QSharedPointer>
#include <QString>
#include <QUrl>
#include <QVariant>

#include "kdeconnectcore_export.h"

class FileTransferJob;

class KDECONNECTCORE_EXPORT NetworkPacket
{
    Q_GADGET
    Q_PROPERTY(QString id READ id MEMBER m_id)
    Q_PROPERTY(QString type READ type MEMBER m_type)
    Q_PROPERTY(QVariantMap body READ body MEMBER m_body)
    Q_PROPERTY(QVariantMap payloadTransferInfo READ payloadTransferInfo MEMBER m_payloadTransferInfo)
    Q_PROPERTY(qint64 payloadSize READ payloadSize MEMBER m_payloadSize)

public:
    const static int s_protocolVersion;

    explicit NetworkPacket(const QString &type = QString(), const QVariantMap &body = {});
    NetworkPacket(const NetworkPacket &other) = default; // Copy constructor, required for QMetaType and queued signals
    NetworkPacket &operator=(const NetworkPacket &other) = default;

    QByteArray serialize() const;
    static bool unserialize(const QByteArray &json, NetworkPacket *out);

    inline QString id() const
    {
        return m_id;
    }
    inline QString type() const
    {
        return m_type;
    }
    QVariantMap body() const
    {
        return m_body;
    }

    // Get and set info from body. Note that id and type can not be accessed through these.
    template<typename T>
    T get(const QString &key, const T &defaultValue = {}) const
    {
        return m_body.value(key, QVariant::fromValue(defaultValue)).template value<T>(); // Important note: Awesome template syntax is awesome
    }
    template<typename T>
    void set(const QString &key, const T &value)
    {
        m_body[key] = QVariant(value);
    }
    bool has(const QString &key) const
    {
        return m_body.contains(key);
    }

    QSharedPointer<QIODevice> payload() const
    {
        return m_payload;
    }
    void setPayload(const QSharedPointer<QIODevice> &device, qint64 payloadSize)
    {
        m_payload = device;
        m_payloadSize = payloadSize;
        Q_ASSERT(m_payloadSize >= -1);
    }
    bool hasPayload() const
    {
        return (m_payloadSize != 0);
    }
    qint64 payloadSize() const
    {
        return m_payloadSize;
    } //-1 means it is an endless stream
    FileTransferJob *createPayloadTransferJob(const QUrl &destination) const;

    // To be called by a particular DeviceLink
    QVariantMap payloadTransferInfo() const
    {
        return m_payloadTransferInfo;
    }
    void setPayloadTransferInfo(const QVariantMap &map)
    {
        m_payloadTransferInfo = map;
    }
    bool hasPayloadTransferInfo() const
    {
        return !m_payloadTransferInfo.isEmpty();
    }

private:
    QString m_id;
    QString m_type;
    QVariantMap m_body;

    QSharedPointer<QIODevice> m_payload;
    qint64 m_payloadSize;
    QVariantMap m_payloadTransferInfo;
};

KDECONNECTCORE_EXPORT QDebug operator<<(QDebug s, const NetworkPacket &pkg);
Q_DECLARE_METATYPE(NetworkPacket)

#endif // NETWORKPACKET_H
