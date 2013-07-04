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
#include <qbytearray.h>
#include <qdatastream.h>
#include <QDebug>
#include <sstream>
#include <string>
#include <qjson/serializer.h>
#include <iostream>
#include <ctime>
#include <qjson/qobjecthelper.h>

const static int CURRENT_PACKAGE_VERSION = 1;

NetworkPackage::NetworkPackage(QString type)
{
    mId = time(NULL);
    mType = type;
    mVersion = CURRENT_PACKAGE_VERSION;
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
    if (!ok) qDebug() << "Serialization error:" << serializer.errorMessage();

    return json;
}

void NetworkPackage::unserialize(QByteArray a, NetworkPackage* np)
{
    //Json -> QVariant
    QJson::Parser parser;
    bool ok;
    QVariantMap variant = parser.parse(a, &ok).toMap();
    if (!ok) {
        qDebug() << "Unserialization error:" << parser.errorLine() << parser.errorString();
        np->setVersion(-1);
    }

    if (np->version() > CURRENT_PACKAGE_VERSION) {
        qDebug() << "Warning: package version " << np->version() << " greater than supported version " << CURRENT_PACKAGE_VERSION;
    }


    //QVariant -> Object
    //NetworkPackage np;
    //QJSon json(a);
    //np.mId = json["id"];
    //np.mType = json["type"];
    //np.mBody = json["body"];
    QJson::QObjectHelper::qvariant2qobject(variant,np);
}

