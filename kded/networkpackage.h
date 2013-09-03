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
#include <QStringList>
#include <QtCrypto>

#include <qjson/parser.h>

#include "default_args.h"

class EncryptedNetworkPackage;

class NetworkPackage : public QObject
{
    Q_OBJECT
    Q_PROPERTY( QString id READ id WRITE setId )
    Q_PROPERTY( QString type READ type WRITE setType )
    Q_PROPERTY( QVariantMap body READ body WRITE setBody )

public:

    const static QCA::EncryptionAlgorithm EncryptionAlgorithm;
    const static int ProtocolVersion;

    NetworkPackage(const QString& type);

    static void createIdentityPackage(NetworkPackage*);

    QByteArray serialize() const;
    static bool unserialize(const QByteArray& json, NetworkPackage* out);

    void encrypt(QCA::PublicKey& key);
    bool decrypt(QCA::PrivateKey& key, NetworkPackage* out) const;

    const QString& id() const { return mId; }
    const QString& type() const { return mType; }
    QVariantMap& body() { return mBody; }

    //Get and set info from body. Note that id and type can not be accessed through these.
    template<typename T> T get(const QString& key, const T& defaultValue = default_arg<T>::get()) const {
        return mBody.value(key,defaultValue).template value<T>(); //Important note: Awesome template syntax is awesome
    }
    template<typename T> void set(const QString& key, const T& value) { mBody[key] = QVariant(value); }
    bool has(const QString& key) const { return mBody.contains(key); }

private:
    void setId(const QString& id) { mId = id; }
    void setType(const QString& t) { mType = t; }
    void setBody(const QVariantMap& b) { mBody = b; }

    QString mId;
    QString mType;
    QVariantMap mBody;

};

//Set specialization need this awesome-to-the-max syntax:
//template<> inline void NetworkPackage::set<QStringList>(const QString& key, const QStringList& value) { mBody[key] = QVariant(value); }

#endif // NETWORKPACKAGE_H
