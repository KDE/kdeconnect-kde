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

#include <QByteArray>
#include <QDataStream>
#include <QHostInfo>
#include <QSslKey>
#include <QDateTime>
#include <QtCrypto>
#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

#include "dbushelper.h"
#include "filetransferjob.h"
#include "pluginloader.h"

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
    np->set("deviceType", "desktop"); //TODO: Detect laptop, tablet, phone...
    np->set("protocolVersion",  NetworkPackage::ProtocolVersion);
    np->set("SupportedIncomingInterfaces", PluginLoader::instance()->incomingInterfaces().join(","));
    np->set("SupportedOutgoingInterfaces", PluginLoader::instance()->outgoingInterfaces().join(","));

    //kDebug(kdeconnect_kded()) << "createIdentityPackage" << np->serialize();
}

QByteArray NetworkPackage::serialize() const
{
    //Object -> QVariant
    //QVariantMap variant;
    //variant["id"] = mId;
    //variant["type"] = mType;
    //variant["body"] = mBody;
    QVariantMap variant = QJson::QObjectHelper::qobject2qvariant(this);

    if (hasPayload()) {
        //kDebug(kdeconnect_kded()) << "Serializing payloadTransferInfo";
        variant["payloadSize"] = payloadSize();
        variant["payloadTransferInfo"] = mPayloadTransferInfo;
    }

    //QVariant -> json
    bool ok;
    QJson::Serializer serializer;
    QByteArray json = serializer.serialize(variant,&ok);
    if (!ok) {
        kDebug(debugArea()) << "Serialization error:" << serializer.errorMessage();
    } else {
        if (!isEncrypted()) {
            //kDebug(kDebugArea) << "Serialized package:" << json;
        }
        json.append('\n');
    }

    return json;
}

bool NetworkPackage::unserialize(const QByteArray& a, NetworkPackage* np)
{
    //Json -> QVariant
    QJson::Parser parser;
    bool ok;
    QVariantMap variant = parser.parse(a, &ok).toMap();
    if (!ok) {
        kDebug(debugArea()) << "Unserialization error:" << a;
        return false;
    }

    //QVariant -> Object
    QJson::QObjectHelper::qvariant2qobject(variant, np);

    if (!np->isEncrypted()) {
        //kDebug(kDebugArea) << "Unserialized: " << a;
    }

    np->mPayloadSize = variant["payloadSize"].toInt(); //Will return 0 if was not present, which is ok
    if (np->mPayloadSize == -1) {
        np->mPayloadSize = np->get<int>("size", -1);
    }
    np->mPayloadTransferInfo = variant["payloadTransferInfo"].toMap(); //Will return an empty qvariantmap if was not present, which is ok

    //Ids containing characters that are not allowed as dbus paths would make app crash
    if (np->mBody.contains("deviceId"))
    {
        QString deviceId = np->get<QString>("deviceId");
        DbusHelper::filterNonExportableCharacters(deviceId);
        np->set("deviceId", deviceId);
    }

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

FileTransferJob* NetworkPackage::createPayloadTransferJob(const KUrl& destination) const
{
    return new FileTransferJob(payload(), payloadSize(), destination);
}

