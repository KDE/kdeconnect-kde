/*
 * SPDX-FileCopyrightText: 2015 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef DBUSHELPERS_H
#define DBUSHELPERS_H

#include <KLocalizedString>
#include <QDBusPendingReply>
#include <QTextStream>

template<typename T>
Q_REQUIRED_RESULT T blockOnReply(QDBusPendingReply<T> reply)
{
    reply.waitForFinished();
    if (reply.isError()) {
        QTextStream(stderr) << i18n("error: ") << reply.error().message() << endl;
        exit(1);
    }
    return reply.value();
}

void blockOnReply(QDBusPendingReply<void> reply)
{
    reply.waitForFinished();
    if (reply.isError()) {
        QTextStream(stderr) << i18n("error: ") << reply.error().message() << endl;
        exit(1);
    }
}

#endif
