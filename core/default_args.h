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

#ifndef DEFAULTARG_H
#define DEFAULTARG_H

#include <QString>

template<class T>
struct default_arg {
    static T get(); //Not defined for any other value
};

//bool -> false
template<>
struct default_arg<bool> {
    static bool get() { return false; }
};

//int -> -1
template<>
struct default_arg<int> {
    static int get() { return -1; }
};

//QByteArray-> empty qbytearray
template<>
struct default_arg<QByteArray> {
    static QByteArray get() { return QByteArray(); }
};

//QStrings -> empty string
template<>
struct default_arg<QString> {
    static QString get() { return QString(); }
};

template<class T>
struct default_arg<T*> {
    static T* get() { return NULL;}
};


#endif // DEFAULTARG_H
