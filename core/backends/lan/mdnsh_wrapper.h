/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECT_MDNSH_WRAPPER_H
#define KDECONNECT_MDNSH_WRAPPER_H

#include <QDeadlineTimer>
#include <QHash>
#include <QHostAddress>
#include <QMap>
#include <QSocketNotifier>
#include <QString>
#include <QVector>

#include "kdeconnectcore_export.h"

/**
 * A Qt wrapper for the mdns.h header-only library
 * from https://github.com/mjansson/mdns
 */
namespace MdnshWrapper
{
class KDECONNECTCORE_EXPORT Discoverer : public QObject
{
    Q_OBJECT

public:
    struct MdnsService {
        QString name; // The instance-name part in "<instance-name>._<service-type>._tcp.local."
        QString serviceType; // ie: "_<service-type>._tcp.local."
        uint16_t port = 0;
        QHostAddress address; // IPv4 if available, IPv6 otherwise
        QMap<QString, QString> txtRecords;
        uint32_t ttl = 0; // Seconds this information is valid for, 0 if the service is going away
    };

    // serviceType must be of the form "_<service-type>._<tcp/udp>.local"
    void startDiscovering(const QString &serviceType);
    void stopDiscovering();

    void sendQuery(const QString &serviceType);

    // Emits serviceFound for matching services, suppressing duplicate responses in a short burst
    void handleResponse(const MdnshWrapper::Discoverer::MdnsService &service);

Q_SIGNALS:
    void serviceFound(const MdnshWrapper::Discoverer::MdnsService &service);

private:
    struct CachedService {
        MdnsService service;
        QDeadlineTimer expires;
    };

    int listenForQueryResponses();
    void stopListeningForQueryResponses();

    QString wantedServiceType;
    QHash<QString, CachedService> cachedServices; // by lowercase instance name
    QVector<QSocketNotifier *> responseSocketNotifiers;
};

class KDECONNECTCORE_EXPORT Announcer : public QObject
{
    Q_OBJECT

public:
    struct AnnouncedInfo {
        QByteArray serviceType; // ie: "_<service-type>._tcp.local."
        QByteArray serviceInstance; // ie: "<instance-name>._<service-type>._tcp.local."
        QByteArray hostname; // ie: "<hostname>.local."
        QVector<QHostAddress> addressesV4;
        QVector<QHostAddress> addressesV6;
        uint16_t port;
        QHash<QByteArray, QByteArray> txtRecords;
    };

    // serviceType must be of the form "_<service-type>._<tcp/udp>.local"
    Announcer(const QString &instanceName, const QString &serviceType, uint16_t port);

    void putTxtRecord(const QString &key, const QString &value)
    {
        self.txtRecords[key.toLatin1()] = value.toLatin1();
    }

    void setPort(uint16_t port)
    {
        self.port = port;
    }

    void startAnnouncing();
    void stopAnnouncing();

    void sendMulticastAnnounce(bool isGoodbye);

Q_SIGNALS:
    // Responses received via multicast on the mDNS port, which the Discoverer doesn't listen to
    void responseReceived(const MdnshWrapper::Discoverer::MdnsService &service);

public Q_SLOTS:
    // notify that network interfaces changed since the creation of this Announcer
    void onNetworkChange()
    {
        detectHostAddresses();
    }

private:
    int listenForQueries();
    void stopListeningForQueries();

    void detectHostAddresses();

    AnnouncedInfo self;
    QHash<uint, QVector<QHostAddress>> addressesByInterface;

    QSocketNotifier *socketNotifier = nullptr;
    QSocketNotifier *socketNotifierV6 = nullptr;
};

} // namespace MdnshWrapper

#endif
