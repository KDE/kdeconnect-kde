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

const QString kServiceType = QStringLiteral("_kdeconnect._udp.local");

MdnsDiscovery::MdnsDiscovery(LanLinkProvider *lanLinkProvider)
    : mdnsAnnouncer(KdeConnectConfig::instance().deviceId(), kServiceType, LanLinkProvider::UDP_PORT)
{
    KdeConnectConfig &config = KdeConnectConfig::instance();
    mdnsAnnouncer.putTxtRecord(QStringLiteral("id"), config.deviceId());
    mdnsAnnouncer.putTxtRecord(QStringLiteral("name"), config.name());
    mdnsAnnouncer.putTxtRecord(QStringLiteral("type"), config.deviceType().toString());
    mdnsAnnouncer.putTxtRecord(QStringLiteral("protocol"), QString::number(NetworkPacket::s_protocolVersion));

    connect(&mdnsDiscoverer, &MdnsWrapper::Discoverer::serviceFound, this, [lanLinkProvider](const MdnsWrapper::Discoverer::MdnsService &service) {
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
    onStop();
}

void MdnsDiscovery::onStart()
{
    mdnsAnnouncer.startAnnouncing();
    mdnsDiscoverer.startDiscovering(kServiceType);
}

void MdnsDiscovery::onStop()
{
    mdnsAnnouncer.stopAnnouncing();
    mdnsDiscoverer.stopDiscovering();
}

void MdnsDiscovery::onNetworkChange()
{
    mdnsAnnouncer.onNetworkChange();
    mdnsDiscoverer.stopDiscovering();
    mdnsDiscoverer.startDiscovering(kServiceType);
}

#include "moc_mdnsdiscovery.cpp"
