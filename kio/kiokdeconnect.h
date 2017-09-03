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

#ifndef KIOKDECONNECT_H
#define KIOKDECONNECT_H

#include <QObject>
#include <QLoggingCategory>

#include <kio/slavebase.h>

#include "interfaces/dbusinterfaces.h"

Q_DECLARE_LOGGING_CATEGORY(KDECONNECT_KIO)

class KioKdeconnect : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

public:
    KioKdeconnect(const QByteArray& pool, const QByteArray& app);

    void get(const QUrl& url) override;
    void listDir(const QUrl& url) override;
    void stat(const QUrl& url) override;

    void setHost(const QString& constHostname, quint16 port, const QString& user, const QString& pass) override;

    void listAllDevices(); //List all devices exported by m_dbusInterface
    void listDevice(); //List m_currentDevice


private:

    /**
     * Contains the ID of the current device or is empty when no device is set.
     */
    QString m_currentDevice;

    /**
     * KDED DBus interface, used to communicate to the daemon since we need some status (like connected)
     */
    DaemonDbusInterface* m_dbusInterface;

};

#endif
