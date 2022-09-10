/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "notifyingapplication.h"

#include <QDataStream>
#include <QDebug>

QDataStream &operator<<(QDataStream &out, const NotifyingApplication &app)
{
    out << app.name << app.icon << app.active << app.blacklistExpression.pattern();
    return out;
}

QDataStream &operator>>(QDataStream &in, NotifyingApplication &app)
{
    QString pattern;
    in >> app.name;
    in >> app.icon;
    in >> app.active;
    in >> pattern;
    app.blacklistExpression.setPattern(pattern);
    return in;
}

QDebug operator<<(QDebug dbg, const NotifyingApplication &a)
{
    dbg.nospace() << "{ name=" << a.name << ", icon=" << a.icon << ", active=" << a.active << ", blacklistExpression =" << a.blacklistExpression << " }";
    return dbg.space();
}
