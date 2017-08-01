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

#include "networkpackage.h"
#include "core_debug.h"

#include <QMetaObject>
#include <QMetaProperty>
#include <QByteArray>
#include <QDataStream>
#include <QDateTime>
#include <QJsonDocument>
#include <QDebug>

#include "dbushelper.h"
#include "filetransferjob.h"
#include "pluginloader.h"
#include "kdeconnectconfig.h"

QDebug operator<<(QDebug s, const NetworkPackage& pkg)
{
    s.nospace() << "NetworkPackage(" << pkg.type() << ':' << pkg.body();
    if (pkg.hasPayload()) {
        s.nospace() << ":withpayload";
    }
    s.nospace() << ')';
    return s.space();
}

const int NetworkPackage::ProtocolVersion = 7;

NetworkPackage::NetworkPackage(const QString& type, const QVariantMap &body)
    : mId(QString::number(QDateTime::currentMSecsSinceEpoch()))
    , mType(type)
    , mBody(body)
    , mPayload()
    , mPayloadSize(0)
{
}

void NetworkPackage::createIdentityPackage(NetworkPackage* np)
{
    KdeConnectConfig* config = KdeConnectConfig::instance();
    np->mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    np->mType = PACKAGE_TYPE_IDENTITY;
    np->mPayload = QSharedPointer<QIODevice>();
    np->mPayloadSize = 0;
    np->set(QStringLiteral("deviceId"), config->deviceId());
    np->set(QStringLiteral("deviceName"), config->name());
    np->set(QStringLiteral("deviceType"), config->deviceType());
    np->set(QStringLiteral("protocolVersion"),  NetworkPackage::ProtocolVersion);
    np->set(QStringLiteral("incomingCapabilities"), PluginLoader::instance()->incomingCapabilities());
    np->set(QStringLiteral("outgoingCapabilities"), PluginLoader::instance()->outgoingCapabilities());

    //qCDebug(KDECONNECT_CORE) << "createIdentityPackage" << np->serialize();
}

QByteArray NetworkPackage::serialize() const
{
    //Object -> QVariant
    QVariantMap variant;
    variant[QStringLiteral("id")] = mId;
    variant[QStringLiteral("type")] = mType;
    variant[QStringLiteral("body")] = mBody;
    if (hasPayload()) {
        variant[QStringLiteral("payloadSize")] = mPayloadSize;
        variant[QStringLiteral("payloadTransferInfo")] = mPayloadTransferInfo;
    }

    //QVariant -> json
    auto jsonDocument = QJsonDocument::fromVariant(variant);
    QByteArray json = jsonDocument.toJson(QJsonDocument::Compact);
    if (json.isEmpty()) {
        qCDebug(KDECONNECT_CORE) << "Serialization error:";
    } else {
        //qDebug() << "Serialized package:" << json;
        json.append('\n');
    }

    return json;
}

bool NetworkPackage::unserialize(const QByteArray& a, NetworkPackage* np)
{
    //Json -> QVariant
    QJsonParseError parseError;
    auto parser = QJsonDocument::fromJson(a, &parseError);
    if (parser.isNull()) {
        qCDebug(KDECONNECT_CORE) << "Unserialization error:" << parseError.errorString();
        return false;
    }

    auto variant = parser.toVariant().toMap();
    np->mId = variant["id"].toInt();
    np->mType = variant["type"].toString();
    np->mBody = variant["body"].toMap();
    np->mPayloadSize = variant[QStringLiteral("payloadSize")].toInt(); //Will return 0 if was not present, which is ok
    np->mPayloadTransferInfo = variant[QStringLiteral("payloadTransferInfo")].toMap(); //Will return an empty qvariantmap if was not present, which is ok

    //HACK: Ids containing characters that are not allowed as dbus paths would make app crash
    if (np->mBody.contains(QStringLiteral("deviceId")))
    {
        QString deviceId = np->get<QString>(QStringLiteral("deviceId"));
        DbusHelper::filterNonExportableCharacters(deviceId);
        np->set(QStringLiteral("deviceId"), deviceId);
    }

    return true;

}

FileTransferJob* NetworkPackage::createPayloadTransferJob(const QUrl &destination) const
{
    return new FileTransferJob(payload(), payloadSize(), destination);
}

