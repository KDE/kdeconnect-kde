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

    Q_INVOKABLE QObject* create() {
        if (m_f0) return m_f0();
        return nullptr;
    }
    
    Q_INVOKABLE QObject* create(const QVariant& arg1) {
        if (m_f1) return m_f1(arg1);
        return nullptr;
    }
    
    Q_INVOKABLE QObject* create(const QVariant& arg1, const QVariant& arg2) {
        if (m_f2) return m_f2(arg1, arg2);
        return nullptr;
    }
    
private:
    Func0 m_f0;
    Func1 m_f1;
    Func2 m_f2;
};


#endif
