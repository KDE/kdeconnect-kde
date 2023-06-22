/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mdnsdiscovery.h"

#include "core_debug.h"
#include "kdeconnectconfig.h"
#include "lanlinkprovider.h"

#include <KDNSSD/DNSSD/PublicService>
#include <KDNSSD/DNSSD/RemoteService>
#include <KDNSSD/DNSSD/ServiceBrowser>

const QString kServiceName = QStringLiteral("_kdeconnect._udp");

MdnsDiscovery::MdnsDiscovery(LanLinkProvider *lanLinkProvider)
    : lanLinkProvider(lanLinkProvider)
{
    switch (KDNSSD::ServiceBrowser::isAvailable()) {
    case KDNSSD::ServiceBrowser::Stopped:
        qWarning() << "mDNS or Avahi daemons are not running, mDNS discovery not available";
        break;
    case KDNSSD::ServiceBrowser::Working:
        qCDebug(KDECONNECT_CORE) << "mDNS discovery is available";
        break;
    case KDNSSD::ServiceBrowser::Unsupported:
        qWarning() << "mDNS discovery not available (library built without DNS-SD support)";
        break;
    }
}

MdnsDiscovery::~MdnsDiscovery()
{
    stopAnnouncing();
    stopDiscovering();
}

void MdnsDiscovery::startAnnouncing()
{
    if (m_publisher != nullptr) {
        qCDebug(KDECONNECT_CORE) << "MDNS already announcing";
        return;
    }
    qCDebug(KDECONNECT_CORE) << "MDNS start announcing";

    KdeConnectConfig &config = KdeConnectConfig::instance();

    m_publisher = new KDNSSD::PublicService(config.deviceId(), kServiceName, LanLinkProvider::UDP_PORT, QStringLiteral("local"));
    m_publisher->setParent(this);

    // We can't fit the device certificate in this field, so this is not enough info to create a Device and won't be used.
    QMap<QString, QByteArray> data;
    data[QStringLiteral("id")] = config.deviceId().toUtf8();
    data[QStringLiteral("name")] = config.name().toUtf8();
    data[QStringLiteral("type")] = config.deviceType().toString().toUtf8();
    data[QStringLiteral("protocol")] = QString::number(NetworkPacket::s_protocolVersion).toUtf8();
    m_publisher->setTextData(data);

    connect(m_publisher, &KDNSSD::PublicService::published, [](bool successful) {
        if (successful) {
            qCDebug(KDECONNECT_CORE) << "MDNS published successfully";
        } else {
            qWarning() << "MDNS failed to publish";
        }
    });

    m_publisher->publishAsync();
}

void MdnsDiscovery::stopAnnouncing()
{
    if (m_publisher != nullptr) {
        qCDebug(KDECONNECT_CORE) << "MDNS stop announcing";
        delete m_publisher;
        m_publisher = nullptr;
    }
}

void MdnsDiscovery::startDiscovering()
{
    if (m_serviceBrowser != nullptr) {
        qCDebug(KDECONNECT_CORE) << "MDNS already discovering";
        return;
    }
    qCDebug(KDECONNECT_CORE) << "MDNS start discovering";

    m_serviceBrowser = new KDNSSD::ServiceBrowser(kServiceName, true);

    connect(m_serviceBrowser, &KDNSSD::ServiceBrowser::serviceAdded, [this](KDNSSD::RemoteService::Ptr service) {
        if (KdeConnectConfig::instance().deviceId() == service->serviceName()) {
            qCDebug(KDECONNECT_CORE) << "Discovered myself, ignoring";
            return;
        }
        qCDebug(KDECONNECT_CORE) << "Discovered " << service->serviceName() << " at " << service->hostName();
        QHostAddress address(service->hostName());
        lanLinkProvider->sendUdpIdentityPacket(QList<QHostAddress>{address});
    });

    connect(m_serviceBrowser, &KDNSSD::ServiceBrowser::serviceRemoved, [](KDNSSD::RemoteService::Ptr service) {
        qCDebug(KDECONNECT_CORE) << "Lost " << service->serviceName();
    });

    connect(m_serviceBrowser, &KDNSSD::ServiceBrowser::finished, []() {
        qCDebug(KDECONNECT_CORE) << "Finished discovery";
    });

    m_serviceBrowser->startBrowse();
}

void MdnsDiscovery::stopDiscovering()
{
    if (m_serviceBrowser != nullptr) {
        qCDebug(KDECONNECT_CORE) << "MDNS stop discovering";
        delete m_serviceBrowser;
        m_serviceBrowser = nullptr;
    }
}
