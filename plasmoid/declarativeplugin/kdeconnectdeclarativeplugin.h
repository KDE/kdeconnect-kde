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

#ifndef ANALITZADECLARATIVEPLUGIN_H
#define ANALITZADECLARATIVEPLUGIN_H

#include <QVariant>
#include <QDeclarativeExtensionPlugin>


//FIXME HACK Force some slot to be synchronous 
#include <libkdeconnect/dbusinterfaces.h>

class SyncSftpDbusInterface : public SftpDbusInterface
{
    Q_OBJECT
public:
    SyncSftpDbusInterface(const QString& id) : SftpDbusInterface(id) {}
    ~SyncSftpDbusInterface(){}
    
    Q_INVOKABLE bool isMounted() {
        return SftpDbusInterface::isMounted();
    }
};

class ObjectFactory : public QObject
{
    Q_OBJECT
    
    typedef QObject* (*Func0)();
    typedef QObject* (*Func1)(QVariant);
    typedef QObject* (*Func2)(QVariant, QVariant);
    
public:
    ObjectFactory(QObject* parent, Func0 f0) : QObject(parent), m_f0(f0), m_f1(0), m_f2(0) {}
    ObjectFactory(QObject* parent, Func1 f1) : QObject(parent), m_f0(0), m_f1(f1), m_f2(0) {}
    ObjectFactory(QObject* parent, Func2 f2) : QObject(parent), m_f0(0), m_f1(0), m_f2(f2) {}
    
    virtual ~ObjectFactory() {};
    
    
    Q_INVOKABLE QObject* create() {
        if (m_f0) return m_f0(); return 0;
    }
    
    Q_INVOKABLE QObject* create(QVariant arg1) {
        if (m_f1) return m_f1(arg1);
        return 0;
    }
    
    Q_INVOKABLE QObject* create(QVariant arg1, QVariant arg2) {
        if (m_f2) return m_f2(arg1, arg2);
        return 0;
    }
    
private:
    Func0 m_f0;
    Func1 m_f1;
    Func2 m_f2;
};


class KdeConnectDeclarativePlugin : public QDeclarativeExtensionPlugin
{
    virtual void registerTypes(const char* uri);
    virtual void initializeEngine(QDeclarativeEngine *engine, const char *uri);
};



#endif // ANALITZADECLARATIVEPLUGIN_H
