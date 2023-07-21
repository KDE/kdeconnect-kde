/**
 * SPDX-FileCopyrightText: 2014 Samoilenko Yuri <kinnalru@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#pragma once

#include <QEventLoop>

class MountLoop : public QEventLoop
{
    Q_OBJECT
public:
    MountLoop();

    bool exec(QEventLoop::ProcessEventsFlags flags = QEventLoop::AllEvents);

Q_SIGNALS:
    void result(bool status);

public Q_SLOTS:
    void failed();
    void succeeded();
    void exitWith(bool status);
};
