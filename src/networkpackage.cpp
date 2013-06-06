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
#include <iostream>

NetworkPackage NetworkPackage::fromString(QByteArray s)
{

    NetworkPackage pp;

    //FIXME: How can I do this using Qt?
    std::string stds(std::string(s.data()));
    std::cout << stds << std::endl;

    std::stringstream ss(stds);

    ss >> pp.mId;
    std::cout <<  pp.mId << std::endl;

    ss >> pp.mDeviceId;
    qDebug() << pp.mDeviceId;

    ss >> pp.mTime;

    std::string type;
    ss >> type;
    pp.mType = QString::fromStdString(type);

    int bodyLenght;
    ss >> bodyLenght;
    char c[bodyLenght];
    ss.get(); //Skip ws
    ss.read(c,bodyLenght);
    pp.mBody = QString::fromAscii(c,bodyLenght);
    qDebug() << pp.mBody;

    ss >> pp.mIsCancel;

    return pp;

}


