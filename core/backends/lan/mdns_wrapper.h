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
namespace MdnsWrapper
{

class Discoverer : public QObject
{
    Q_OBJECT

public:
    struct MdnsService {
        QString name;
        uint16_t port;
        QHostAddress address;
        QMap<QString, QString> txtRecords;
    };

    // serviceType must be of the form "_<name>._<tcp/udp>.local"
    void startDiscovering(const QString &serviceType);
    void stopDiscovering();

    void sendQuery(const QString &serviceName);

Q_SIGNALS:
    void serviceFound(const MdnsWrapper::Discoverer::MdnsService &service);

private:
    int listenForQueryResponses();
    void stopListeningForQueryResponses();

    QVector<QSocketNotifier *> responseSocketNotifiers;
};

class Announcer : public QObject
{
    Q_OBJECT

public:
    struct AnnouncedInfo {
        QByteArray serviceType; // ie: "<_service-type>._tcp.local."
        QByteArray serviceInstance; // ie: "<service-name>.<_service-type>._tcp.local."
        QByteArray hostname; // ie: "<hostname>.local."
        QVector<QHostAddress> addressesV4;
        QVector<QHostAddress> addressesV6;
        uint16_t port;
        QHash<QByteArray, QByteArray> txtRecords;
    };

    // serviceType must be of the form "_<name>._<tcp/udp>.local"
    Announcer(const QString &serviceName, const QString &serviceType, uint16_t port);

    void putTxtRecord(const QString &key, const QString &value)
    {
        self.txtRecords[key.toLatin1()] = value.toLatin1();
    }

    void startAnnouncing();
    void stopAnnouncing();

    void sendMulticastAnnounce(bool isGoodbye);

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

    QSocketNotifier *socketNotifier = nullptr;
    QSocketNotifier *socketNotifierV6 = nullptr;
};

} // namespace MdnsWrapper

#endif
