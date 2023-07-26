/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "networkpacket.h"
#include "core_debug.h"

#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QMetaObject>
#include <QMetaProperty>

#include "dbushelper.h"
#include "filetransferjob.h"
#include "kdeconnectconfig.h"
#include "pluginloader.h"

QDebug operator<<(QDebug s, const NetworkPacket &pkg)
{
    s.nospace() << "NetworkPacket(" << pkg.type() << ':' << pkg.body();
    if (pkg.hasPayload()) {
        s.nospace() << ":withpayload";
    }
    s.nospace() << ')';
    return s.space();
}

const int NetworkPacket::s_protocolVersion = 7;

NetworkPacket::NetworkPacket(const QString &type, const QVariantMap &body)
    : m_id(QString::number(QDateTime::currentMSecsSinceEpoch()))
    , m_type(type)
    , m_body(body)
    , m_payload()
    , m_payloadSize(0)
{
}

QByteArray NetworkPacket::serialize() const
{
    // Object -> QVariant
    QVariantMap variant;
    variant.insert(QStringLiteral("id"), m_id);
    variant.insert(QStringLiteral("type"), m_type);
    variant.insert(QStringLiteral("body"), m_body);

    if (hasPayload()) {
        variant.insert(QStringLiteral("payloadSize"), m_payloadSize);
        variant.insert(QStringLiteral("payloadTransferInfo"), m_payloadTransferInfo);
    }

    // QVariant -> json
    auto jsonDocument = QJsonDocument::fromVariant(variant);
    QByteArray json = jsonDocument.toJson(QJsonDocument::Compact);
    if (json.isEmpty()) {
        qCDebug(KDECONNECT_CORE) << "Serialization error:";
    } else {
        /*if (!isEncrypted()) {
            //qCDebug(KDECONNECT_CORE) << "Serialized packet:" << json;
        }*/
        json.append('\n');
    }

    return json;
}

template<class T>
void qvariant2qobject(const QVariantMap &variant, T *object)
{
    for (QVariantMap::const_iterator iter = variant.begin(); iter != variant.end(); ++iter) {
        const int propertyIndex = T::staticMetaObject.indexOfProperty(iter.key().toLatin1().data());
        if (propertyIndex < 0) {
            qCWarning(KDECONNECT_CORE) << "missing property" << object << iter.key();
            continue;
        }

        QMetaProperty property = T::staticMetaObject.property(propertyIndex);
        bool ret = property.writeOnGadget(object, *iter);
        if (!ret) {
            qCWarning(KDECONNECT_CORE) << "couldn't set" << object << "->" << property.name() << '=' << *iter;
        }
    }
}

bool NetworkPacket::unserialize(const QByteArray &a, NetworkPacket *np)
{
    // Json -> QVariant
    QJsonParseError parseError;
    auto parser = QJsonDocument::fromJson(a, &parseError);
    if (parser.isNull()) {
        qCDebug(KDECONNECT_CORE) << "Unserialization error:" << parseError.errorString();
        return false;
    }

    auto variant = parser.toVariant().toMap();
    qvariant2qobject(variant, np);

    np->m_payloadTransferInfo = variant[QStringLiteral("payloadTransferInfo")].toMap(); // Will return an empty qvariantmap if was not present, which is ok

    // Ids containing characters that are not allowed as dbus paths would make app crash
    if (np->m_body.contains(QStringLiteral("deviceId"))) {
        QString deviceId = np->get<QString>(QStringLiteral("deviceId"));
        DBusHelper::filterNonExportableCharacters(deviceId);
        np->set(QStringLiteral("deviceId"), deviceId);
    }

    return true;
}

FileTransferJob *NetworkPacket::createPayloadTransferJob(const QUrl &destination) const
{
    return new FileTransferJob(this, destination);
}

#include "moc_networkpacket.cpp"
