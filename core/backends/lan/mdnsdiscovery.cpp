/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mdnsdiscovery.h"

#include "core_debug.h"
#include "kdeconnectconfig.h"
#include "lanlinkprovider.h"

#include "mdns_wrapper.h"

#include <QHostInfo>
const char *kServiceName = "_kdeconnect._udp.local";

MdnsDiscovery::MdnsDiscovery(LanLinkProvider *lanLinkProvider)
    : lanLinkProvider(lanLinkProvider)
{
    connect(&mdnsWrapper, &MdnsWrapper::serviceFound, [lanLinkProvider](const MdnsWrapper::MdnsService &service) {
        if (KdeConnectConfig::instance().deviceId() == service.name) {
            qCDebug(KDECONNECT_CORE) << "Discovered myself, ignoring";
            return;
        }
        lanLinkProvider->sendUdpIdentityPacket(QList<QHostAddress>{service.address});
        qCDebug(KDECONNECT_CORE) << "Discovered" << service.name << "at" << service.address;
    });
}

MdnsDiscovery::~MdnsDiscovery()
{
    stopAnnouncing();
    stopDiscovering();
}

QString hostName = QHostInfo::localHostName();

void MdnsDiscovery::startAnnouncing()
{
    // mdnsWrapper.startAnnouncing(hostName.toLatin1().data(), kServiceName, LanLinkProvider::UDP_PORT);

    /*
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
            qCWarning(KDECONNECT_CORE) << "MDNS failed to publish";
        }
    });

    m_publisher->publishAsync();
    */
}

void MdnsDiscovery::stopAnnouncing()
{
    // mdnsWrapper.stopAnnouncing();

    // if (m_publisher != nullptr) {
    //     qCDebug(KDECONNECT_CORE) << "MDNS stop announcing";
    //     delete m_publisher;
    //     m_publisher = nullptr;
    // }
}

void MdnsDiscovery::startDiscovering()
{
    mdnsWrapper.startDiscovering(kServiceName);
}

void MdnsDiscovery::stopDiscovering()
{
    mdnsWrapper.stopDiscovering();
}
