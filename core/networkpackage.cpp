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

const int NetworkPackage::s_protocolVersion = 7;

NetworkPackage::NetworkPackage(const QString& type, const QVariantMap& body)
    : m_id(QString::number(QDateTime::currentMSecsSinceEpoch()))
    , m_type(type)
    , m_body(body)
    , m_payload()
    , m_payloadSize(0)
{
}

void NetworkPackage::createIdentityPackage(NetworkPackage* np)
{
    KdeConnectConfig* config = KdeConnectConfig::instance();
    np->m_id = QString::number(QDateTime::currentMSecsSinceEpoch());
    np->m_type = PACKAGE_TYPE_IDENTITY;
    np->m_payload = QSharedPointer<QIODevice>();
    np->m_payloadSize = 0;
    np->set(QStringLiteral("deviceId"), config->deviceId());
    np->set(QStringLiteral("deviceName"), config->name());
    np->set(QStringLiteral("deviceType"), config->deviceType());
    np->set(QStringLiteral("protocolVersion"),  NetworkPackage::s_protocolVersion);
    np->set(QStringLiteral("incomingCapabilities"), PluginLoader::instance()->incomingCapabilities());
    np->set(QStringLiteral("outgoingCapabilities"), PluginLoader::instance()->outgoingCapabilities());

    //qCDebug(KDECONNECT_CORE) << "createIdentityPackage" << np->serialize();
}

template<class T>
QVariantMap qobject2qvariant(const T* object)
{
    QVariantMap map;
    auto metaObject = T::staticMetaObject;
    for(int i = metaObject.propertyOffset(); i < metaObject.propertyCount(); ++i) {
        QMetaProperty prop = metaObject.property(i);
        map.insert(QString::fromLatin1(prop.name()), prop.readOnGadget(object));
    }

    return map;
}

QByteArray NetworkPackage::serialize() const
{
    //Object -> QVariant
    //QVariantMap variant;
    //variant["id"] = mId;
    //variant["type"] = mType;
    //variant["body"] = mBody;
    QVariantMap variant = qobject2qvariant(this);

    if (hasPayload()) {
        //qCDebug(KDECONNECT_CORE) << "Serializing payloadTransferInfo";
        variant[QStringLiteral("payloadSize")] = payloadSize();
        variant[QStringLiteral("payloadTransferInfo")] = m_payloadTransferInfo;
    }

    //QVariant -> json
    auto jsonDocument = QJsonDocument::fromVariant(variant);
    QByteArray json = jsonDocument.toJson(QJsonDocument::Compact);
    if (json.isEmpty()) {
        qCDebug(KDECONNECT_CORE) << "Serialization error:";
    } else {
        /*if (!isEncrypted()) {
            //qCDebug(KDECONNECT_CORE) << "Serialized package:" << json;
        }*/
        json.append('\n');
    }

    return json;
}

template <class T>
void qvariant2qobject(const QVariantMap& variant, T* object)
{
    for ( QVariantMap::const_iterator iter = variant.begin(); iter != variant.end(); ++iter )
    {
        const int propertyIndex = T::staticMetaObject.indexOfProperty(iter.key().toLatin1());
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
    qvariant2qobject(variant, np);

    np->m_payloadSize = variant[QStringLiteral("payloadSize")].toInt(); //Will return 0 if was not present, which is ok
    if (np->m_payloadSize == -1) {
        np->m_payloadSize = np->get<int>(QStringLiteral("size"), -1);
    }
    np->m_payloadTransferInfo = variant[QStringLiteral("payloadTransferInfo")].toMap(); //Will return an empty qvariantmap if was not present, which is ok

    //Ids containing characters that are not allowed as dbus paths would make app crash
    if (np->m_body.contains(QStringLiteral("deviceId")))
    {
        QString deviceId = np->get<QString>(QStringLiteral("deviceId"));
        DbusHelper::filterNonExportableCharacters(deviceId);
        np->set(QStringLiteral("deviceId"), deviceId);
    }

    return true;

}

FileTransferJob* NetworkPackage::createPayloadTransferJob(const QUrl& destination) const
{
    return new FileTransferJob(payload(), payloadSize(), destination);
}

