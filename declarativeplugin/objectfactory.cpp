/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "objectfactory.h"

QObject *ObjectFactory::create()
{
    if (m_f0)
        return m_f0();
    return nullptr;
}

QObject *ObjectFactory::create(const QVariant &arg1)
{
    if (m_f1)
        return m_f1(arg1);
    return nullptr;
}

QObject *ObjectFactory::create(const QVariant &arg1, const QVariant &arg2)
{
    if (m_f2)
        return m_f2(arg1, arg2);
    return nullptr;
}

#include "moc_objectfactory.cpp"
