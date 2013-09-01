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

const static int CURRENT_PACKAGE_VERSION = 2;

NetworkPackage::NetworkPackage(const QString& type)
{
    mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    mType = type;
    mBody = QVariantMap();
    mVersion = CURRENT_PACKAGE_VERSION;
    mEncrypted = false;
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

void NetworkPackage::unserialize(const QByteArray& a, NetworkPackage* np)
{
    qDebug() << "Unserialize: " << a;

    //Json -> QVariant
    QJson::Parser parser;
    bool ok;
    QVariantMap variant = parser.parse(a, &ok).toMap();
    if (!ok) {
        qDebug() << "Unserialization error:" << parser.errorLine() << parser.errorString();
        np->setVersion(-1);
    }

    //QVariant -> Object
    QJson::QObjectHelper::qvariant2qobject(variant,np);

    if (np->version() > CURRENT_PACKAGE_VERSION) {
        qDebug() << "Warning: package version" << np->version() << "is greater than supported version" << CURRENT_PACKAGE_VERSION;
    }

}

void NetworkPackage::encrypt (QCA::PublicKey& key)
{
    qDebug() << key.toDER() << key.canEncrypt();
    
    QByteArray serialized = serialize();
    QByteArray data = key.encrypt(serialized, QCA::EME_PKCS1v15).toByteArray();

    mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    mType = "kdeconnect.encrypted";
    mBody = QVariantMap();
    mBody["data"] = data.toBase64();
    mVersion = CURRENT_PACKAGE_VERSION;
    mEncrypted = true;

}

void NetworkPackage::decrypt (QCA::PrivateKey& key, NetworkPackage* out) const
{
    QByteArray encryptedJson = QByteArray::fromBase64(get<QByteArray>("data"));

    QCA::SecureArray decryptedJson;
    key.decrypt(encryptedJson, &decryptedJson, QCA::EME_PKCS1v15);

    unserialize(decryptedJson.toByteArray(), out);
}



void NetworkPackage::createIdentityPackage(NetworkPackage* np)
{
    KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    QString id = config->group("myself").readEntry<QString>("id","");
    np->mId = time(NULL);
    np->mType = PACKAGE_TYPE_IDENTITY;
    np->mVersion = CURRENT_PACKAGE_VERSION;
    np->set("deviceId", id);
    np->set("deviceName", QHostInfo::localHostName());

    //qDebug() << "createIdentityPackage" << np->serialize();
}


