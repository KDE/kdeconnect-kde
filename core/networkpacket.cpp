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

const int NetworkPacket::s_protocolVersion = 8;

NetworkPacket::NetworkPacket(const QString &type, const QVariantMap &body)
    : m_type(type)
    , m_body(body)
    , m_payload()
    , m_payloadSize(0)
{
}

QByteArray NetworkPacket::serialize() const
{
    QJsonObject obj;
    obj.insert(QLatin1String("id"), QDateTime::currentMSecsSinceEpoch());
    obj.insert(QLatin1String("type"), m_type);
    obj.insert(QLatin1String("body"), QJsonObject::fromVariantMap(m_body));

    if (hasPayload()) {
        obj.insert(QLatin1String("payloadSize"), m_payloadSize);
        obj.insert(QLatin1String("payloadTransferInfo"), QJsonObject::fromVariantMap(m_payloadTransferInfo));
    }

    QByteArray json = QJsonDocument(obj).toJson(QJsonDocument::Compact);
    if (json.isEmpty()) {
        qCDebug(KDECONNECT_CORE) << "Serialization error:";
    } else {
        json.append('\n');
    }

    return json;
}

bool NetworkPacket::unserialize(const QByteArray &a, NetworkPacket *np)
{
    // Json -> QVariant
    QJsonParseError parseError;
    QJsonDocument obj = QJsonDocument::fromJson(a, &parseError);
    if (obj.isNull()) {
        qCDebug(KDECONNECT_CORE) << "Unserialization error:" << parseError.errorString();
        return false;
    }

    np->m_type = obj[QStringLiteral("type")].toString();
    np->m_body = obj[QStringLiteral("body")].toObject().toVariantMap();
    np->m_payloadSize = obj[QStringLiteral("payloadSize")].toInteger();
    // Will return an empty qvariantmap if was not present, which is ok
    np->m_payloadTransferInfo = obj[QLatin1String("payloadTransferInfo")].toObject().toVariantMap();

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
