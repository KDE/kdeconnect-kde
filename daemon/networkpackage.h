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

#include <QString>
#include <QVariant>

class NetworkPackage
{

public:

    NetworkPackage() {
        mId = 3;
        //TODO
    }

    static NetworkPackage fromString(QByteArray);
    QByteArray toString() const;

    long id() const { return mId; }
    const QString& deviceId() const { return mDeviceId; }
    const QString& type() const { return mType; }
    const QString& body() const { return mBody; }
    bool isCancel() const { return mIsCancel; }

    void setId(long id) { mId = id; }
    void setDeviceId(const QString& id) { mDeviceId = id; }
    void setType(const QString& t) { mType = t; }
    void setBody(const QString& b) { mBody = b; }
    void setCancel(bool b) { mIsCancel = b; }

private:

    long mId;
    long mTime;
    QString mDeviceId;
    QString mType;
    QString mBody;
    QVariant mExtra;
    bool mIsCancel;

};

#endif // NETWORKPACKAGE_H
