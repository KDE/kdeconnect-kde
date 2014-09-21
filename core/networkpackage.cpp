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

#include <KSharedConfig>
#include <KConfigGroup>

#include <QMetaObject>
#include <QMetaProperty>
#include <QByteArray>
#include <QDataStream>
#include <QHostInfo>
#include <QSslKey>
#include <QDateTime>
#include <qjsondocument.h>
#include <QtCrypto>

#include "filetransferjob.h"

const QCA::EncryptionAlgorithm NetworkPackage::EncryptionAlgorithm = QCA::EME_PKCS1v15;
const int NetworkPackage::ProtocolVersion = 5;

NetworkPackage::NetworkPackage(const QString& type)
{
    mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    mType = type;
    mBody = QVariantMap();
    mPayload = QSharedPointer<QIODevice>();
    mPayloadSize = 0;
}

void NetworkPackage::createIdentityPackage(NetworkPackage* np)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    const QString id = config->group("myself").readEntry<QString>("id","");
    np->mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    np->mType = PACKAGE_TYPE_IDENTITY;
    np->mPayload = QSharedPointer<QIODevice>();
    np->mPayloadSize = 0;
    np->set("deviceId", id);
    np->set("deviceName", qgetenv("USER") + "@" + QHostInfo::localHostName());
    np->set("protocolType", "desktop"); //TODO: Detect laptop, tablet, phone...
    np->set("protocolVersion",  NetworkPackage::ProtocolVersion);

    //kDebug(kdeconnect_kded()) << "createIdentityPackage" << np->serialize();
}

QVariantMap qobject2qvairant(const QObject* object)
{
    QVariantMap map;
    auto metaObject = object->metaObject();
    for(int i = metaObject->propertyOffset(); i < metaObject->propertyCount(); ++i) {
        const char *name = metaObject->property(i).name();
        map.insert(QString::fromLatin1(name), object->property(name));
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
    QVariantMap variant = qobject2qvairant(this);

    if (hasPayload()) {
        //kDebug(kdeconnect_kded()) << "Serializing payloadTransferInfo";
        variant["payloadSize"] = payloadSize();
        variant["payloadTransferInfo"] = mPayloadTransferInfo;
    }

    //QVariant -> json
    bool ok;
    auto jsonDocument = QJsonDocument::fromVariant(variant);
    QByteArray json = jsonDocument.toJson(QJsonDocument::Compact);
    if (json.isEmpty()) {
        kDebug(debugArea()) << "Serialization error:";
    } else {
        if (!isEncrypted()) {
            //kDebug(kDebugArea) << "Serialized package:" << json;
        }
        json.append('\n');
    }

    return json;
}

void qvariant2qobject(const QVariantMap& variant, QObject* object)
{
    for ( QVariantMap::const_iterator iter = variant.begin(); iter != variant.end(); ++iter )
    {
        QVariant property = object->property( iter.key().toLatin1() );
        Q_ASSERT( property.isValid() );
        if ( property.isValid() )
        {
            QVariant value = iter.value();
            if ( value.canConvert( property.type() ) )
            {
                value.convert( property.type() );
                object->setProperty( iter.key().toLatin1(), value );
            } else if ( QString( QLatin1String("QVariant") ).compare( QLatin1String( property.typeName() ) ) == 0) {
                object->setProperty( iter.key().toLatin1(), value );
            }
        }
    }
}

bool NetworkPackage::unserialize(const QByteArray& a, NetworkPackage* np)
{
    //Json -> QVariant
    QJsonParseError parseError;
    auto parser = QJsonDocument::fromJson(a, &parseError);
    if (parser.isNull()) {
        kDebug(debugArea()) << "Unserialization error:" << parseError.errorString();
        return false;
    }

    auto variant = parser.toVariant().toMap();
    qvariant2qobject(variant, np);

    if (!np->isEncrypted()) {
        //kDebug(kDebugArea) << "Unserialized: " << a;
    }

    np->mPayloadSize = variant["payloadSize"].toInt(); //Will return 0 if was not present, which is ok
    if (np->mPayloadSize == -1) {
        np->mPayloadSize = np->get<int>("size", -1);
    }
    np->mPayloadTransferInfo = variant["payloadTransferInfo"].toMap(); //Will return an empty qvariantmap if was not present, which is ok

    return true;

}

void NetworkPackage::encrypt(QCA::PublicKey& key)
{

    QByteArray serialized = serialize();

    int chunkSize = key.maximumEncryptSize(NetworkPackage::EncryptionAlgorithm);

    QStringList chunks;
    while (!serialized.isEmpty()) {
        const QByteArray chunk = serialized.left(chunkSize);
        serialized = serialized.mid(chunkSize);
        const QByteArray encryptedChunk = key.encrypt(chunk, NetworkPackage::EncryptionAlgorithm).toByteArray();
        chunks.append( encryptedChunk.toBase64() );
    }

    //kDebug(kdeconnect_kded()) << chunks.size() << "chunks";

    mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    mType = PACKAGE_TYPE_ENCRYPTED;
    mBody = QVariantMap();
    mBody["data"] = chunks;

}

bool NetworkPackage::decrypt(QCA::PrivateKey& key, NetworkPackage* out) const
{

    const QStringList& chunks = mBody["data"].toStringList();

    QByteArray decryptedJson;
    Q_FOREACH(const QString& chunk, chunks) {
        const QByteArray encryptedChunk = QByteArray::fromBase64(chunk.toAscii());
        QCA::SecureArray decryptedChunk;
        bool success = key.decrypt(encryptedChunk, &decryptedChunk, NetworkPackage::EncryptionAlgorithm);
        if (!success) {
            return false;
        }
        decryptedJson.append(decryptedChunk.toByteArray());
    }

    bool success = unserialize(decryptedJson, out);

    if (!success) {
        return false;
    }

    if (hasPayload()) {
        out->mPayload = mPayload;
    }

    return true;

}

FileTransferJob* NetworkPackage::createPayloadTransferJob(const QUrl &destination) const
{
    return new FileTransferJob(payload(), payloadSize(), destination);
}

