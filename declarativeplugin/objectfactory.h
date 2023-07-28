/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef QOBJECT_FACTORY_H
#define QOBJECT_FACTORY_H

#include <QObject>
#include <QVariant>

// Wraps a factory function with QObject class to be exposed to qml context as named factory

class ObjectFactory : public QObject
{
    Q_OBJECT

    typedef QObject *(*Func0)();
    typedef QObject *(*Func1)(const QVariant &);
    typedef QObject *(*Func2)(const QVariant &, const QVariant &);

public:
    ObjectFactory(QObject *parent, Func0 f0)
        : QObject(parent)
        , m_f0(f0)
        , m_f1(nullptr)
        , m_f2(nullptr)
    {
    }
    ObjectFactory(QObject *parent, Func1 f1)
        : QObject(parent)
        , m_f0(nullptr)
        , m_f1(f1)
        , m_f2(nullptr)
    {
    }
    ObjectFactory(QObject *parent, Func2 f2)
        : QObject(parent)
        , m_f0(nullptr)
        , m_f1(nullptr)
        , m_f2(f2)
    {
    }

    Q_INVOKABLE QObject *create();
    Q_INVOKABLE QObject *create(const QVariant &arg1);

    Q_INVOKABLE QObject *create(const QVariant &arg1, const QVariant &arg2);

private:
    Func0 m_f0;
    Func1 m_f1;
    Func2 m_f2;
};

#endif
