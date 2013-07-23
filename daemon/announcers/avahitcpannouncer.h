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

#ifndef AVAHITCPANNOUNCER_H
#define AVAHITCPANNOUNCER_H

#include <QObject>
#include <QTcpServer>

#include <KDE/DNSSD/PublicService>

#include "announcer.h"
#include "netaddress.h"


class AvahiTcpAnnouncer
    : public Announcer
{
    Q_OBJECT

public:
    AvahiTcpAnnouncer();
    ~AvahiTcpAnnouncer();

    QString name() { return "AvahiTcpAnnouncer"; }
    int priority() { return PRIORITY_HIGH + 1; }

    void setDiscoverable(bool b);

private Q_SLOTS:
    void newConnection();
    void deviceLinkDestroyed(QObject*);
    void dataReceived();

private:
    DNSSD::PublicService* service;
    QTcpServer* mServer;

    QMap<QString, DeviceLink*> links;

};

#endif
