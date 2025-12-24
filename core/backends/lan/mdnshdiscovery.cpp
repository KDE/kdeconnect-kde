/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mdnshdiscovery.h"

#include "core_debug.h"
#include "kdeconnectconfig.h"
#include "lanlinkprovider.h"

#include "mdnsh_wrapper.h"

const QString kServiceType = QStringLiteral("_kdeconnect._udp.local");

MdnshDiscovery::MdnshDiscovery(LanLinkProvider *lanLinkProvider)
    : mdnsAnnouncer(KdeConnectConfig::instance().deviceId(), kServiceType, lanLinkProvider->tcpPort())
{
    KdeConnectConfig &config = KdeConnectConfig::instance();
    mdnsAnnouncer.putTxtRecord(QStringLiteral("id"), config.deviceId());
    mdnsAnnouncer.putTxtRecord(QStringLiteral("name"), config.name());
    mdnsAnnouncer.putTxtRecord(QStringLiteral("type"), config.deviceType().toString());
    mdnsAnnouncer.putTxtRecord(QStringLiteral("protocol"), QString::number(NetworkPacket::s_protocolVersion));

    connect(&mdnsDiscoverer, &MdnshWrapper::Discoverer::serviceFound, this, [lanLinkProvider](const MdnshWrapper::Discoverer::MdnsService &service) {
        if (KdeConnectConfig::instance().deviceId() == service.name) {
            qCDebug(KDECONNECT_CORE) << "Discovered myself, ignoring";
            return;
        }
        // TODO: For protocol v8, we can skip ahead and open a TCP connection
        //       instead of sending a UDP packet and waiting for the other end
        //       to send the TCP connection to us, since we already have all the
        //       info we need to start a connection (ip, port, device id and protocol
        //       version) and the remaining identity info is exchanged later.
        lanLinkProvider->sendUdpIdentityPacket(QList<QHostAddress>{service.address});
        qCDebug(KDECONNECT_CORE) << "Discovered" << service.name << "at" << service.address;
    });
}

MdnshDiscovery::~MdnshDiscovery()
{
    onStop();
}

void MdnshDiscovery::onStart()
{
    mdnsAnnouncer.startAnnouncing();
    mdnsDiscoverer.startDiscovering(kServiceType);
}

void MdnshDiscovery::onStop()
{
    mdnsAnnouncer.stopAnnouncing();
    mdnsDiscoverer.stopDiscovering();
}

void MdnshDiscovery::onNetworkChange()
{
    mdnsAnnouncer.onNetworkChange();
    mdnsDiscoverer.stopDiscovering();
    mdnsDiscoverer.startDiscovering(kServiceType);
}

#include "moc_mdnshdiscovery.cpp"
