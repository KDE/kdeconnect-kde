/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECT_MDNS_WRAPPER_H
#define KDECONNECT_MDNS_WRAPPER_H

#include <QHostAddress>
#include <QMap>
#include <QSocketNotifier>
#include <QString>
#include <QVector>

/*
 * A Qt wrapper for the mdns.h header-only library
 * from https://github.com/mjansson/mdns
 */
class MdnsWrapper : public QObject
{
    Q_OBJECT

public:
    struct MdnsService {
        QString serviceName;
        int port;
        QHostAddress address;
        QMap<QString, QString> txtRecords;
    };

    int startAnnouncing(const char *hostname, const char *service_name, int service_port);
    void stopAnnouncing();

    void startDiscovering(const char *serviceType);
    void stopDiscovering();

    void sendQuery(const char *serviceName);

Q_SIGNALS:
    void serviceFound(const MdnsService &service);

private:
    int listenForQueryResponses();
    void stopListeningForQueryResponses();

    QVector<QSocketNotifier *> responseSocketNotifiers;
};

#endif
