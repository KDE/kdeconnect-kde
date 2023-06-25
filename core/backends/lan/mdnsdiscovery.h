/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#ifndef KDECONNECT_MDNS_DISCOVERY_H
#define KDECONNECT_MDNS_DISCOVERY_H

#include <QObject>

#include "kdeconnectcore_export.h"

class LanLinkProvider;
namespace KDNSSD
{
class PublicService;
class ServiceBrowser;
};

class KDECONNECTCORE_EXPORT MdnsDiscovery : public QObject
{
    Q_OBJECT

public:
    explicit MdnsDiscovery(LanLinkProvider *parent);
    ~MdnsDiscovery();

    void startDiscovering();
    void stopDiscovering();

    void stopAnnouncing();
    void startAnnouncing();

private:
    LanLinkProvider *lanLinkProvider = nullptr;
    KDNSSD::PublicService *m_publisher = nullptr;
    KDNSSD::ServiceBrowser *m_serviceBrowser = nullptr;
};

#endif // KDECONNECT_SERVER_H
