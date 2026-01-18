/**
 * SPDX-FileCopyrightText: 2025 Albert Vaca <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "avahidiscovery.h"

#include "core_debug.h"
#include "dbushelpers.h"
#include "kdeconnectconfig.h"
#include "lanlinkprovider.h"

const QString kAvahiDbusService = QStringLiteral("org.freedesktop.Avahi");

const QString kKdeConnectServiceType = QStringLiteral("_kdeconnect._udp");

void AvahiServiceBrowserDeleter::operator()(OrgFreedesktopAvahiServiceBrowserInterface *ptr) const
{
    if (ptr) {
        auto freeReply = ptr->Free();
        freeReply.waitForFinished();
        if (freeReply.isError()) {
            qCWarning(KDECONNECT_CORE) << "Avahi: Could not free browser:" << freeReply.error();
        }
        delete ptr;
    }
}

void AvahiEntryGroupDeleter::operator()(OrgFreedesktopAvahiEntryGroupInterface *ptr) const
{
    if (ptr) {
        auto freeReply = ptr->Free();
        freeReply.waitForFinished();
        if (freeReply.isError()) {
            qCWarning(KDECONNECT_CORE) << "Avahi: Could not free entry group:" << freeReply.error();
        }
        delete ptr;
    }
}

enum {
    AVAHI_SERVER_INVALID,
    AVAHI_SERVER_REGISTERING,
    AVAHI_SERVER_RUNNING,
    AVAHI_SERVER_COLLISION,
    AVAHI_SERVER_FAILURE,
};

enum {
    AVAHI_ENTRY_GROUP_UNCOMMITTED,
    AVAHI_ENTRY_GROUP_REGISTERING,
    AVAHI_ENTRY_GROUP_ESTABLISHED,
    AVAHI_ENTRY_GROUP_COLLISION,
    AVAHI_ENTRY_GROUP_FAILURE,
};

bool AvahiDiscovery::hasAvahiDaemonRunning()
{
    OrgFreedesktopAvahiServer2Interface avahiServerInterface(kAvahiDbusService, QStringLiteral("/"), QDBusConnection::systemBus());
    return avahiServerInterface.isValid();
}

AvahiDiscovery::AvahiDiscovery(LanLinkProvider *lanLinkProvider)
    : m_avahiServerInterface(kAvahiDbusService, QStringLiteral("/"), QDBusConnection::systemBus())
    , lanLinkProvider(lanLinkProvider)
{
    connect(&m_avahiServerInterface, &OrgFreedesktopAvahiServer2Interface::StateChanged, this, [](int state, const QString &error) {
        if (state == AVAHI_SERVER_COLLISION) {
            qCWarning(KDECONNECT_CORE) << "Avahi: server collision" << error;
        } else if (state == AVAHI_SERVER_FAILURE) {
            qCWarning(KDECONNECT_CORE) << "Avahi: server failure" << error;
        }
    });
}

AvahiDiscovery::~AvahiDiscovery()
{
    onStop();
}

void AvahiDiscovery::onStart()
{
    startAnnouncing();
    startDiscovering();
}

void AvahiDiscovery::onStop()
{
    stopAnnouncing();
    stopDiscovering();
}

void AvahiDiscovery::onNetworkChange()
{
    stopDiscovering();
    startDiscovering();
}

void AvahiDiscovery::startAnnouncing()
{
    if (!m_avahiServerInterface.isValid()) {
        qCWarning(KDECONNECT_CORE) << "Avahi: Could not connect to Avahi server interface" << m_avahiServerInterface.lastError();
        return;
    }

    if (m_entryGroupInterface) {
        qCWarning(KDECONNECT_CORE) << "Avahi: already announcing";
        return;
    }

    QDBusPendingReply<QDBusObjectPath> pathReply = m_avahiServerInterface.EntryGroupNew();
    pathReply.waitForFinished();
    if (pathReply.isError()) {
        qCWarning(KDECONNECT_CORE) << "Avahi: Could not create entry group:" << pathReply.error();
        return;
    }
    m_entryGroupInterface.reset(new OrgFreedesktopAvahiEntryGroupInterface(kAvahiDbusService, pathReply.value().path(), QDBusConnection::systemBus()));

    if (!m_entryGroupInterface->isValid()) {
        qCWarning(KDECONNECT_CORE) << "Avahi: Could not connect to entity group" << m_entryGroupInterface->lastError();
        m_entryGroupInterface.reset();
        return;
    }

    connect(m_entryGroupInterface.get(), &OrgFreedesktopAvahiEntryGroupInterface::StateChanged, this, [](int state, const QString &error) {
        if (state == AVAHI_ENTRY_GROUP_COLLISION) {
            qCWarning(KDECONNECT_CORE) << "Avahi: entry group collision" << error;
        } else if (state == AVAHI_ENTRY_GROUP_FAILURE) {
            qCWarning(KDECONNECT_CORE) << "Avahi: entry group failure" << error;
        }
    });

    KdeConnectConfig &config = KdeConnectConfig::instance();
    QByteArrayList txtRecords;
    txtRecords.append((QByteArray("id=") + config.deviceId().toUtf8()));
    txtRecords.append((QByteArray("name=") + config.name().toUtf8()));
    txtRecords.append((QByteArray("type=") + config.deviceType().toString().toUtf8()));
    txtRecords.append((QByteArray("protocol=") + QString::number(NetworkPacket::s_protocolVersion).toUtf8()));

    auto addServiceReply = m_entryGroupInterface->AddService(-1, // interface: AVAHI_IF_UNSPEC
                                                             -1, // protocol: AVAHI_PROTO_UNSPEC
                                                             64, // flags: AVAHI_PUBLISH_UPDATE
                                                             config.deviceId(), // name
                                                             kKdeConnectServiceType, // type
                                                             QString(), // domain
                                                             QString(), // host
                                                             lanLinkProvider->tcpPort(), // port
                                                             txtRecords);

    addServiceReply.waitForFinished();
    if (addServiceReply.isError()) {
        qCWarning(KDECONNECT_CORE) << "Avahi: Could not add service:" << addServiceReply.error();
        m_entryGroupInterface.reset();
        return;
    }
    auto commitReply = m_entryGroupInterface->Commit();
    commitReply.waitForFinished();
    if (commitReply.isError()) {
        qCWarning(KDECONNECT_CORE) << "Avahi: Could not commit service:" << commitReply.error();
        m_entryGroupInterface.reset();
        return;
    }
    qCDebug(KDECONNECT_CORE) << "Avahi: Started announcing for device" << config.deviceId();
}

void AvahiDiscovery::stopAnnouncing()
{
    if (!m_entryGroupInterface) {
        qCWarning(KDECONNECT_CORE) << "Avahi: already not announcing";
        return;
    }

    m_entryGroupInterface.reset();
    qCDebug(KDECONNECT_CORE) << "Avahi: Stopped announcing";
}

void AvahiDiscovery::startDiscovering()
{
    if (!m_avahiServerInterface.isValid()) {
        qCWarning(KDECONNECT_CORE) << "Avahi: Could not connect to server interface" << m_avahiServerInterface.lastError();
        return;
    }

    if (m_serviceBrowserInterface) {
        qCWarning(KDECONNECT_CORE) << "Avahi: already discovering";
        return;
    }

    QDBusPendingReply<QDBusObjectPath> pathReply = m_avahiServerInterface.ServiceBrowserPrepare(-1, // interface: AVAHI_IF_UNSPEC
                                                                                                -1, // protocol: AVAHI_PROTO_UNSPEC
                                                                                                kKdeConnectServiceType, // type
                                                                                                QString(), // domain
                                                                                                0 // flags
    );
    pathReply.waitForFinished();
    if (pathReply.isError()) {
        qCWarning(KDECONNECT_CORE) << "Avahi: Could not create entry group:" << pathReply.error();
        return;
    }

    m_serviceBrowserInterface.reset(new OrgFreedesktopAvahiServiceBrowserInterface(kAvahiDbusService, pathReply.value().path(), QDBusConnection::systemBus()));

    if (!m_serviceBrowserInterface->isValid()) {
        qCWarning(KDECONNECT_CORE) << "Avahi: Could not connect to browser interface" << m_serviceBrowserInterface->lastError();
        m_serviceBrowserInterface.reset();
        return;
    }

    connect(m_serviceBrowserInterface.get(),
            &OrgFreedesktopAvahiServiceBrowserInterface::ItemNew,
            this,
            [this](int interface, int protocol, const QString &name, const QString &type, const QString &domain) {
                if (KdeConnectConfig::instance().deviceId() == name) {
                    qCDebug(KDECONNECT_CORE) << "Avahi: Discovered myself, ignoring";
                    return;
                }

                auto service = m_avahiServerInterface.ResolveService(interface, protocol, name, type, domain, -1, 0);
                setWhenAvailableN(
                    service,
                    [this, name](const QDBusPendingReply<int, int, QString, QString, QString, QString, int, QString, ushort, QByteArrayList, uint> &service) {
                        if (service.isError()) {
                            qCWarning(KDECONNECT_CORE) << "Avahi: Could not resolve service for" << name << ":" << service.error();
                            return;
                        }

                        // args: 0: iface, 1: protocol, 3: name, 4: type, 5: domain, 6: host, 7: aprotocol, 8: address, 9: port, 10: txt, 11: flags
                        const QString address = service.argumentAt<7>();

                        // TODO: For protocol v8, we can skip ahead and open a TCP connection
                        //       instead of sending a UDP packet and waiting for the other end
                        //       to send the TCP connection to us, since we already have all the
                        //       info we need to start a connection (ip, port, device id and protocol
                        //       version) and the remaining identity info is exchanged later.

                        qCDebug(KDECONNECT_CORE) << "Avahi: Discovered" << name << "at" << address;
                        lanLinkProvider->sendUdpIdentityPacket(QList<QHostAddress>{QHostAddress(address)});
                    },
                    m_serviceBrowserInterface.get());
            });

    connect(m_serviceBrowserInterface.get(), &OrgFreedesktopAvahiServiceBrowserInterface::Failure, this, [](const QString &error) {
        qCWarning(KDECONNECT_CORE) << "Avahi: discovery failure" << error;
    });

    auto startReply = m_serviceBrowserInterface->Start();
    startReply.waitForFinished();
    if (startReply.isError()) {
        qCWarning(KDECONNECT_CORE) << "Avahi: could not start discovering" << startReply.error();
        m_serviceBrowserInterface.reset();
        return;
    }
    qCDebug(KDECONNECT_CORE) << "Avahi: Started discovering";
}

void AvahiDiscovery::stopDiscovering()
{
    if (!m_serviceBrowserInterface) {
        qCWarning(KDECONNECT_CORE) << "Avahi: already not discovering";
        return;
    }

    m_serviceBrowserInterface.reset();
    qCDebug(KDECONNECT_CORE) << "Avahi: Stopped discovery";
}

#include "moc_avahidiscovery.cpp"
