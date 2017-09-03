/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
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

#ifndef NETWORKPACKAGE_H
#define NETWORKPACKAGE_H

#include "networkpackagetypes.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QIODevice>
//#include <QtCrypto>
#include <QSharedPointer>
#include <QUrl>

#include "kdeconnectcore_export.h"

class FileTransferJob;

class KDECONNECTCORE_EXPORT NetworkPackage
{
    Q_GADGET
    Q_PROPERTY( QString id READ id WRITE setId )
    Q_PROPERTY( QString type READ type WRITE setType )
    Q_PROPERTY( QVariantMap body READ body WRITE setBody )
    Q_PROPERTY( QVariantMap payloadTransferInfo READ payloadTransferInfo WRITE setPayloadTransferInfo )
    Q_PROPERTY( qint64 payloadSize READ payloadSize WRITE setPayloadSize )

public:

    //const static QCA::EncryptionAlgorithm EncryptionAlgorithm;
    const static int s_protocolVersion;

    explicit NetworkPackage(const QString& type, const QVariantMap& body = {});

    static void createIdentityPackage(NetworkPackage*);

    QByteArray serialize() const;
    static bool unserialize(const QByteArray& json, NetworkPackage* out);

    const QString& id() const { return m_id; }
    const QString& type() const { return m_type; }
    QVariantMap& body() { return m_body; }
    const QVariantMap& body() const { return m_body; }

    //Get and set info from body. Note that id and type can not be accessed through these.
    template<typename T> T get(const QString& key, const T& defaultValue = {}) const {
        return m_body.value(key,defaultValue).template value<T>(); //Important note: Awesome template syntax is awesome
    }
    template<typename T> void set(const QString& key, const T& value) { m_body[key] = QVariant(value); }
    bool has(const QString& key) const { return m_body.contains(key); }

    QSharedPointer<QIODevice> payload() const { return m_payload; }
    void setPayload(const QSharedPointer<QIODevice>& device, qint64 payloadSize) { m_payload = device; m_payloadSize = payloadSize; Q_ASSERT(m_payloadSize >= -1); }
    bool hasPayload() const { return (m_payloadSize != 0); }
    qint64 payloadSize() const { return m_payloadSize; } //-1 means it is an endless stream
    FileTransferJob* createPayloadTransferJob(const QUrl& destination) const;

    //To be called by a particular DeviceLink
    QVariantMap payloadTransferInfo() const { return m_payloadTransferInfo; }
    void setPayloadTransferInfo(const QVariantMap& map) { m_payloadTransferInfo = map; }
    bool hasPayloadTransferInfo() const { return !m_payloadTransferInfo.isEmpty(); }

private:

    void setId(const QString& id) { m_id = id; }
    void setType(const QString& t) { m_type = t; }
    void setBody(const QVariantMap& b) { m_body = b; }
    void setPayloadSize(qint64 s) { m_payloadSize = s; }

    QString m_id;
    QString m_type;
    QVariantMap m_body;
	
    QSharedPointer<QIODevice> m_payload;
    qint64 m_payloadSize;
    QVariantMap m_payloadTransferInfo;

};

KDECONNECTCORE_EXPORT QDebug operator<<(QDebug s, const NetworkPackage& pkg);

#endif // NETWORKPACKAGE_H
