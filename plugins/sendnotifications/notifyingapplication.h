/**
 * SPDX-FileCopyrightText: 2015 Holger Kaelberer <holger.k@elberer.de>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QRegularExpression>

struct NotifyingApplication {
    QString name;
    QString icon;
    bool active;
    QRegularExpression blacklistExpression;

    bool operator==(const NotifyingApplication &other) const
    {
        return (name == other.name);
    }
};

Q_DECLARE_METATYPE(NotifyingApplication);

QDataStream &operator<<(QDataStream &out, const NotifyingApplication &app);
QDataStream &operator>>(QDataStream &in, NotifyingApplication &app);
QDebug operator<<(QDebug dbg, const NotifyingApplication &a);
