/**
 * SPDX-FileCopyrightText: 2018 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef ABSTRACTREMOTEINPUT_H
#define ABSTRACTREMOTEINPUT_H

#include <QObject>

#include <core/networkpacket.h>

class AbstractRemoteInput : public QObject
{
    Q_OBJECT
public:
    explicit AbstractRemoteInput(QObject *parent = nullptr);

    virtual bool handlePacket(const NetworkPacket &np) = 0;
    virtual bool hasKeyboardSupport()
    {
        return false;
    };
};

#endif
