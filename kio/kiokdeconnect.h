/**
 * SPDX-FileCopyrightText: 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KIOKDECONNECT_H
#define KIOKDECONNECT_H

#include <QObject>

#include <kio/slavebase.h>

#include "dbusinterfaces.h"

class KioKdeconnect : public QObject, public KIO::SlaveBase
{
    Q_OBJECT

public:
    KioKdeconnect(const QByteArray& pool, const QByteArray& app);

    void get(const QUrl& url) override;
    void listDir(const QUrl& url) override;
    void stat(const QUrl& url) override;

    void listAllDevices(); //List all devices exported by m_dbusInterface
    void listDevice(const QString& device); //List m_currentDevice


private:

    /**
     * KDED DBus interface, used to communicate to the daemon since we need some status (like connected)
     */
    DaemonDbusInterface* m_dbusInterface;

};

#endif
