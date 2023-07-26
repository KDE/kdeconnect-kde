/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mountloop.h"

MountLoop::MountLoop()
    : QEventLoop()
{
}

bool MountLoop::exec(QEventLoop::ProcessEventsFlags flags)
{
    return QEventLoop::exec(flags) == 0;
}

void MountLoop::failed()
{
    Q_EMIT result(false);
    exit(1);
}

void MountLoop::succeeded()
{
    Q_EMIT result(true);
    exit(0);
}

void MountLoop::exitWith(bool status)
{
    Q_EMIT result(status);
    exit(status ? 0 : 1);
}

#include "moc_mountloop.cpp"
