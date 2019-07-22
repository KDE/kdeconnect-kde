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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef QOBJECT_FACTORY_H
#define QOBJECT_FACTORY_H

#include <QObject>
#include <QVariant>

//Wraps a factory function with QObject class to be exposed to qml context as named factory

class ObjectFactory : public QObject
{
    Q_OBJECT

    typedef QObject* (*Func0)();
    typedef QObject* (*Func1)(const QVariant&);
    typedef QObject* (*Func2)(const QVariant&, const QVariant&);

public:
    ObjectFactory(QObject* parent, Func0 f0) : QObject(parent), m_f0(f0), m_f1(nullptr), m_f2(nullptr) {}
    ObjectFactory(QObject* parent, Func1 f1) : QObject(parent), m_f0(nullptr), m_f1(f1), m_f2(nullptr) {}
    ObjectFactory(QObject* parent, Func2 f2) : QObject(parent), m_f0(nullptr), m_f1(nullptr), m_f2(f2) {}

    ~ObjectFactory() override = default;

    Q_INVOKABLE QObject* create();
    Q_INVOKABLE QObject* create(const QVariant& arg1);

    Q_INVOKABLE QObject* create(const QVariant& arg1, const QVariant& arg2);

private:
    Func0 m_f0;
    Func1 m_f1;
    Func2 m_f2;
};


#endif
