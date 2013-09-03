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
#include <QtCrypto/QtCrypto>

#include <qjson/serializer.h>
#include <qjson/qobjecthelper.h>

const QCA::EncryptionAlgorithm NetworkPackage::EncryptionAlgorithm = QCA::EME_PKCS1v15;
const int NetworkPackage::ProtocolVersion = 3;

NetworkPackage::NetworkPackage(const QString& type)
{
    mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    mType = type;
    mBody = QVariantMap();
}

QByteArray NetworkPackage::serialize() const
{
    //Object -> QVariant
    //QVariantMap variant;
    //variant["id"] = mId;
    //variant["type"] = mType;
    //variant["body"] = mBody;
    QVariantMap variant = QJson::QObjectHelper::qobject2qvariant(this);

    //QVariant -> json
    bool ok;
    QJson::Serializer serializer;
    QByteArray json = serializer.serialize(variant,&ok);
    if (!ok) {
        qDebug() << "Serialization error:" << serializer.errorMessage();
    } else {
        qDebug() << "Serialized package:" << json;
        json.append('\n');
    }

    return json;
}

bool NetworkPackage::unserialize(const QByteArray& a, NetworkPackage* np)
{
    qDebug() << "Unserialize: " << a;

    //Json -> QVariant
    QJson::Parser parser;
    bool ok;
    QVariantMap variant = parser.parse(a, &ok).toMap();
    if (!ok) {
        qDebug() << "Unserialization error:" << parser.errorLine() << parser.errorString();
        return false;
    }

    //QVariant -> Object
    QJson::QObjectHelper::qvariant2qobject(variant,np);

    return true;

}

void NetworkPackage::encrypt (QCA::PublicKey& key)
{

    QByteArray serialized = serialize();

    int chunkSize = key.maximumEncryptSize(NetworkPackage::EncryptionAlgorithm);

    QStringList chunks;
    while (!serialized.isEmpty()) {
        QByteArray chunk = serialized.left(chunkSize);
        serialized = serialized.mid(chunkSize);
        QByteArray encryptedChunk = key.encrypt(chunk, NetworkPackage::EncryptionAlgorithm).toByteArray();
        chunks.append( encryptedChunk.toBase64() );
    }

    qDebug() << chunks.size() << "chunks";

    mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    mType = PACKAGE_TYPE_ENCRYPTED;
    mBody = QVariantMap();
    mBody["data"] = chunks;

}

bool NetworkPackage::decrypt (QCA::PrivateKey& key, NetworkPackage* out) const
{
    const QStringList& chunks = mBody["data"].toStringList();

    QByteArray decryptedJson;
    Q_FOREACH(const QString& chunk, chunks) {
        QByteArray encryptedChunk = QByteArray::fromBase64(chunk.toAscii());
        QCA::SecureArray decryptedChunk;
        bool success = key.decrypt(encryptedChunk, &decryptedChunk, NetworkPackage::EncryptionAlgorithm);
        if (!success) {
            return false;
        }
        decryptedJson.append(decryptedChunk.toByteArray());

    }

    return unserialize(decryptedJson, out);

}



void NetworkPackage::createIdentityPackage(NetworkPackage* np)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    QString id = config->group("myself").readEntry<QString>("id","");
    np->mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    np->mType = PACKAGE_TYPE_IDENTITY;
    np->set("deviceId", id);
    np->set("deviceName", QHostInfo::localHostName());
    np->set("protocolVersion",  NetworkPackage::ProtocolVersion);

    //qDebug() << "createIdentityPackage" << np->serialize();
}


