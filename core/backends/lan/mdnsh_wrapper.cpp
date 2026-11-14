/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mdnsh_wrapper.h"

#include "core_debug.h"

#include "mdns.h"

#include <errno.h>

#include <QHostInfo>
#include <QNetworkInterface>
#include <QSocketNotifier>

namespace MdnshWrapper
{
const char *recordTypeToStr(int rtype)
{
    switch (rtype) {
    case MDNS_RECORDTYPE_PTR:
        return "PTR";
    case MDNS_RECORDTYPE_SRV:
        return "SRV";
    case MDNS_RECORDTYPE_TXT:
        return "TXT";
    case MDNS_RECORDTYPE_A:
        return "A";
    case MDNS_RECORDTYPE_AAAA:
        return "AAAA";
    case MDNS_RECORDTYPE_ANY:
        return "ANY";
    default:
        return "UNKNOWN";
    }
}

const char *entryTypeToStr(int entry)
{
    switch (entry) {
    case MDNS_ENTRYTYPE_QUESTION:
        return "QUESTION";
    case MDNS_ENTRYTYPE_ANSWER:
        return "ANSWER";
    case MDNS_ENTRYTYPE_AUTHORITY:
        return "AUTHORITY";
    case MDNS_ENTRYTYPE_ADDITIONAL:
        return "ADDITIONAL";
    default:
        return "UNKNOWN";
    }
}

static sockaddr_in qHostAddressToSockaddr(QHostAddress hostAddress)
{
    Q_ASSERT(hostAddress.protocol() == QAbstractSocket::IPv4Protocol);
    sockaddr_in socketAddress;
    memset(&socketAddress, 0, sizeof(struct sockaddr_in));
    socketAddress.sin_family = AF_INET;
    socketAddress.sin_addr.s_addr = htonl(hostAddress.toIPv4Address());
    return socketAddress;
}

static sockaddr_in6 qHostAddressToSockaddr6(QHostAddress hostAddress)
{
    Q_ASSERT(hostAddress.protocol() == QAbstractSocket::IPv6Protocol);
    sockaddr_in6 socketAddress;
    memset(&socketAddress, 0, sizeof(struct sockaddr_in6));
    socketAddress.sin6_family = AF_INET6;
    Q_IPV6ADDR ipv6Address = hostAddress.toIPv6Address();
    for (int i = 0; i < 16; ++i) {
        socketAddress.sin6_addr.s6_addr[i] = ipv6Address[i];
    }
    return socketAddress;
}

// Multicast DNS packets can be up to 9000 bytes (RFC 6762 section 17)
static const size_t maxReceivedPacketSize = 9000;

// Records of one received packet, grouped by owner name so packets that contain several services can be split
struct MdnsRecords {
    struct Ptr {
        QString serviceType; // ie: "_<service-type>._tcp.local."
        QString instanceKey; // lowercase "<instance-name>._<service-type>._tcp.local."
        QString name; // The instance-name part
        uint32_t ttl;
    };
    struct Srv {
        QString hostnameKey; // lowercase "<hostname>.local."
        uint16_t port;
    };
    QVector<Ptr> ptrs;
    QHash<QString, Srv> srvs; // by instance
    QHash<QString, QMap<QString, QString>> txts; // by instance
    QHash<QString, QVector<QHostAddress>> addresses; // by hostname
    QHostAddress sender;
    uint32_t senderScopeId = 0;
    Announcer::AnnouncedInfo *announcer = nullptr; // If set, questions are answered with service_callback
};

static int service_callback(int sock,
                            const struct sockaddr *from,
                            size_t addrlen,
                            mdns_entry_type_t entry_type,
                            uint16_t query_id,
                            uint16_t record_type,
                            uint16_t rclass,
                            uint32_t ttl,
                            const void *data,
                            size_t size,
                            size_t name_offset,
                            size_t name_length,
                            size_t record_offset,
                            size_t record_length,
                            void *user_data);

// Callback that collects the records of a response, and passes questions to the Announcer if set
static int query_callback(int sock,
                          const struct sockaddr *from,
                          size_t addrlen,
                          mdns_entry_type_t entry_type,
                          uint16_t query_id,
                          uint16_t record_type,
                          uint16_t rclass,
                          uint32_t ttl,
                          const void *data,
                          size_t size,
                          size_t name_offset,
                          size_t name_length,
                          size_t record_offset,
                          size_t record_length,
                          void *user_data)
{
    MdnsRecords *records = static_cast<MdnsRecords *>(user_data);

    if (entry_type == MDNS_ENTRYTYPE_QUESTION) {
        if (!records->announcer) {
            return 0;
        }
        return service_callback(sock,
                                from,
                                addrlen,
                                entry_type,
                                query_id,
                                record_type,
                                rclass,
                                ttl,
                                data,
                                size,
                                name_offset,
                                name_length,
                                record_offset,
                                record_length,
                                records->announcer);
    }

    // A TTL of zero is a goodbye record. Only the PTR one is needed to know which service is going away.
    if (ttl == 0 && record_type != MDNS_RECORDTYPE_PTR) {
        return 0;
    }

    // qCDebug(KDECONNECT_CORE) << "Received DNS record of type" << recordTypeToStr(record_type) << "from socket" << sock << "with type" <<
    // entryTypeToStr(entry_type);

    if (records->sender.isNull()) {
        records->sender = QHostAddress(from);
        if (from->sa_family == AF_INET6) {
            records->senderScopeId = reinterpret_cast<const sockaddr_in6 *>(from)->sin6_scope_id;
        }
    }

    char ownerBuffer[256];
    const mdns_string_t ownerString = mdns_string_extract(data, size, &name_offset, ownerBuffer, sizeof(ownerBuffer));
    const QString owner = QString::fromLatin1(ownerString.str, ownerString.length);

    switch (record_type) {
    case MDNS_RECORDTYPE_PTR: {
        char instanceBuffer[256];
        const mdns_string_t instance = mdns_record_parse_ptr(data, size, record_offset, record_length, instanceBuffer, sizeof(instanceBuffer));
        mdns_string_pair_t instanceNamePos = mdns_get_next_substring(data, size, record_offset);
        records->ptrs.append({owner,
                              QString::fromLatin1(instance.str, instance.length).toLower(),
                              QString::fromLatin1((char *)data + instanceNamePos.offset, instanceNamePos.length),
                              ttl});
    } break;
    case MDNS_RECORDTYPE_SRV: {
        char hostnameBuffer[256];
        mdns_record_srv_t record = mdns_record_parse_srv(data, size, record_offset, record_length, hostnameBuffer, sizeof(hostnameBuffer));
        records->srvs.insert(owner.toLower(), {QString::fromLatin1(record.name.str, record.name.length).toLower(), record.port});
    } break;
    case MDNS_RECORDTYPE_A: {
        sockaddr_in addr;
        mdns_record_parse_a(data, size, record_offset, record_length, &addr);
        records->addresses[owner.toLower()].append(QHostAddress(ntohl(addr.sin_addr.s_addr)));
    } break;
    case MDNS_RECORDTYPE_AAAA: {
        sockaddr_in6 addr6;
        mdns_record_parse_aaaa(data, size, record_offset, record_length, &addr6);
        records->addresses[owner.toLower()].append(QHostAddress(addr6.sin6_addr.s6_addr));
    } break;
    case MDNS_RECORDTYPE_TXT: {
        mdns_record_txt_t txtRecords[24];
        size_t parsed = mdns_record_parse_txt(data, size, record_offset, record_length, txtRecords, sizeof(txtRecords) / sizeof(mdns_record_txt_t));
        QMap<QString, QString> &txt = records->txts[owner.toLower()];
        for (size_t itxt = 0; itxt < parsed; ++itxt) {
            QString key = QString::fromLatin1(txtRecords[itxt].key.str, txtRecords[itxt].key.length);
            QString value = QString::fromLatin1(txtRecords[itxt].value.str, txtRecords[itxt].value.length);
            txt[key] = value;
        }
    } break;
    default:
        // Ignore unknown record types
        break;
    }

    return 0;
}

// Returns the local interface whose subnet contains the address, if any.
// Multicast mDNS packets are not routed, so the sender is in the subnet of the interface that received them.
static QString interfaceForAddress(const QHostAddress &address)
{
    const QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &iface : interfaces) {
        const auto flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::IsRunning)) {
            continue;
        }
        const QList<QNetworkAddressEntry> entries = iface.addressEntries();
        for (const QNetworkAddressEntry &entry : entries) {
            if (entry.ip().protocol() == address.protocol() && address.isInSubnet(entry.ip(), entry.prefixLength())) {
                return iface.name();
            }
        }
    }
    return QString();
}

// Prefer IPv4, in a local subnet if there are several. Otherwise prefer link-local IPv6, matching the sender if advertised.
static QHostAddress chooseAddress(const QVector<QHostAddress> &addresses, const QHostAddress &sender)
{
    QVector<QHostAddress> ipv4;
    QHostAddress ipv6;
    for (const QHostAddress &address : addresses) {
        if (address.protocol() == QAbstractSocket::IPv4Protocol) {
            ipv4.append(address);
        } else if (ipv6.isNull() || (address.isLinkLocal() && (!ipv6.isLinkLocal() || address == sender))) {
            ipv6 = address;
        }
    }
    if (ipv4.size() > 1) {
        for (const QHostAddress &address : std::as_const(ipv4)) {
            if (!interfaceForAddress(address).isEmpty()) {
                return address;
            }
        }
    }
    return ipv4.isEmpty() ? ipv6 : ipv4.first();
}

// Build one service for each PTR record, using the SRV, TXT and address records that belong to it
static QVector<Discoverer::MdnsService> servicesFromRecords(const MdnsRecords &records, const QString &receivingInterface = QString())
{
    QVector<Discoverer::MdnsService> services;
    for (const MdnsRecords::Ptr &ptr : records.ptrs) {
        Discoverer::MdnsService service;
        service.name = ptr.name;
        service.serviceType = ptr.serviceType;
        service.txtRecords = records.txts.value(ptr.instanceKey);
        service.ttl = ptr.ttl;

        const auto srv = records.srvs.constFind(ptr.instanceKey);
        if (srv != records.srvs.cend()) {
            service.port = srv->port;
            service.address = chooseAddress(records.addresses.value(srv->hostnameKey), records.sender);
        }
        if (service.address.isNull()) {
            service.address = records.sender; // In case we don't receive an A or AAAA record
        }

        if (service.address.isLinkLocal() && service.address.scopeId().isEmpty()) {
            if (records.senderScopeId != 0) {
                service.address.setScopeId(QString::number(records.senderScopeId));
            } else if (!receivingInterface.isEmpty()) {
                service.address.setScopeId(receivingInterface);
            } else if (records.sender.protocol() == QAbstractSocket::IPv4Protocol || !records.sender.isLinkLocal()) {
                service.address.setScopeId(interfaceForAddress(records.sender));
            }
        }

        services.append(service);
    }
    return services;
}

void Discoverer::startDiscovering(const QString &serviceType)
{
    wantedServiceType = serviceType;
    if (!wantedServiceType.endsWith(QLatin1Char('.'))) {
        wantedServiceType.append(QLatin1Char('.'));
    }

    int num_sockets = listenForQueryResponses();
    if (num_sockets <= 0) {
        qCWarning(KDECONNECT_CORE) << "Failed to open any MDNS server sockets";
        return;
    }
    sendQuery(serviceType);
}

void Discoverer::stopDiscovering()
{
    stopListeningForQueryResponses();
    wantedServiceType.clear();
    cachedServices.clear();
}

void Discoverer::stopListeningForQueryResponses()
{
    qCDebug(KDECONNECT_CORE) << "Closing" << responseSocketNotifiers.size() << "sockets";
    for (QSocketNotifier *socketNotifier : std::as_const(responseSocketNotifiers)) {
        mdns_socket_close(socketNotifier->socket());
        delete socketNotifier;
    }
    responseSocketNotifiers.clear();
}

int Discoverer::listenForQueryResponses()
{
    // Open a socket for each interface, keeping the interface name to scope link-local IPv6 addresses
    QHash<int, QString> sockets;
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        int flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::CanMulticast) || (flags & QNetworkInterface::IsLoopBack)) {
            continue;
        }
        for (const QNetworkAddressEntry &ifaceAddress : iface.addressEntries()) {
            QHostAddress sourceAddress = ifaceAddress.ip();
            if (sourceAddress.protocol() == QAbstractSocket::IPv4Protocol && sourceAddress != QHostAddress::LocalHost) {
                qCDebug(KDECONNECT_CORE) << "Opening socket for address" << sourceAddress;
                struct sockaddr_in saddr = qHostAddressToSockaddr(sourceAddress);
                int socket = mdns_socket_open_ipv4(&saddr);
                if (socket >= 0) {
                    sockets.insert(socket, iface.name());
                } else {
                    qCDebug(KDECONNECT_CORE) << "Couldn't open socket";
                }
            } else if (sourceAddress.protocol() == QAbstractSocket::IPv6Protocol && sourceAddress != QHostAddress::LocalHostIPv6) {
                qCDebug(KDECONNECT_CORE) << "Opening socket for address6" << sourceAddress;
                struct sockaddr_in6 saddr = qHostAddressToSockaddr6(sourceAddress);
                int socket = mdns_socket_open_ipv6(&saddr);
                if (socket >= 0) {
                    sockets.insert(socket, iface.name());
                } else {
                    qCDebug(KDECONNECT_CORE) << "Couldn't open socket";
                }
            }
        }
    }

    // Start listening on all sockets
    for (auto it = sockets.cbegin(); it != sockets.cend(); ++it) {
        const QString ifaceName = it.value();
        QSocketNotifier *socketNotifier = new QSocketNotifier(it.key(), QSocketNotifier::Read);
        QObject::connect(socketNotifier, &QSocketNotifier::activated, this, [this, ifaceName](QSocketDescriptor socket) {
            MdnsRecords records;

            static char buffer[maxReceivedPacketSize];
            mdns_query_recv(socket, buffer, sizeof(buffer), query_callback, &records, 0);

            const QVector<MdnsService> services = servicesFromRecords(records, ifaceName);
            for (const MdnsService &service : services) {
                handleResponse(service);
            }
        });
        responseSocketNotifiers.append(socketNotifier);
    }

    qCDebug(KDECONNECT_CORE) << "Opened" << sockets.size() << "sockets to listen for MDNS query responses";

    return sockets.size();
}

void Discoverer::handleResponse(const MdnsService &service)
{
    if (service.serviceType.compare(wantedServiceType, Qt::CaseInsensitive) != 0 || service.name.isEmpty()) {
        return;
    }

    const QString key = service.name.toLower();

    if (service.ttl == 0) {
        cachedServices.remove(key);
        return;
    }

    // Can't connect without a SRV record or an address
    if (service.port == 0 || service.address.isNull()) {
        return;
    }

    cachedServices.removeIf([](const auto &it) {
        return it->expires.hasExpired();
    });

    // Suppress bursts across sockets, but let later announcements retry failed connections.
    // Addresses can differ between responses, and missing TXT records aren't a change.
    auto cached = cachedServices.constFind(key);
    if (cached != cachedServices.cend() && cached->service.port == service.port
        && (service.txtRecords.isEmpty() || cached->service.txtRecords == service.txtRecords)) {
        return;
    }
    cachedServices.insert(key, {service, QDeadlineTimer(1000)});

    Q_EMIT serviceFound(service);
}

void Discoverer::sendQuery(const QString &serviceType)
{
    qCDebug(KDECONNECT_CORE) << "Sending MDNS query for service" << serviceType;

    // Report every service that answers again, even if it's in the cache
    cachedServices.clear();

    mdns_query_t query;
    QByteArray serviceTypeBytes = serviceType.toLatin1();
    query.name = serviceTypeBytes.constData();
    query.length = serviceTypeBytes.length();
    query.type = MDNS_RECORDTYPE_PTR;

    static char buffer[2048];
    for (QSocketNotifier *socketNotifier : std::as_const(responseSocketNotifiers)) {
        int socket = socketNotifier->socket();
        qCDebug(KDECONNECT_CORE) << "Sending mDNS query via socket" << socket;
        int ret = mdns_multiquery_send(socket, &query, 1, buffer, sizeof(buffer), 0);
        if (ret < 0) {
            qCWarning(KDECONNECT_CORE) << "Failed to send mDNS query:" << strerror(errno);
        }
    }
}

const QByteArray dnsSdName = QByteArray("_services._dns-sd._udp.local.");

static mdns_string_t createMdnsString(const QByteArray &str)
{
    return mdns_string_t{str.constData(), (size_t)str.length()};
}

int countCommonLeadingBits(quint32 int1, quint32 int2)
{
    int count = 0;
    while (int1 != 0 && int2 != 0) {
        if ((int1 & 0x80000000) == (int2 & 0x80000000)) {
            count++;
            int1 <<= 1;
            int2 <<= 1;
        } else {
            break;
        }
    }
    return count;
}

static QHostAddress findBestAddressMatchV4(const QVector<QHostAddress> &hostAddresses, const struct sockaddr *fromAddress)
{
    Q_ASSERT(!hostAddresses.empty());
    if (hostAddresses.size() == 1 || fromAddress == nullptr) {
        return hostAddresses[0];
    }

    QHostAddress otherIp = QHostAddress(fromAddress);

    if (otherIp.protocol() != QAbstractSocket::IPv4Protocol) {
        return hostAddresses[0];
    }

    // qDebug() << "I have more than one IP address:" << hostAddresses << "- Finding best match for source IP:" << otherIp;

    QHostAddress matchingIp = hostAddresses[0];
    int matchingBits = -1;
    quint32 rawOtherIp = otherIp.toIPv4Address();
    for (const QHostAddress &ip : hostAddresses) {
        Q_ASSERT(ip.protocol() == QAbstractSocket::IPv4Protocol);
        quint32 rawMyIp = ip.toIPv4Address();
        // Since we don't have the network mask, we just compare the prefixes of the IPs to find the longest match
        int matchingBitsCount = countCommonLeadingBits(rawMyIp, rawOtherIp);
        if (matchingBitsCount > matchingBits) {
            matchingIp = ip;
            matchingBits = matchingBitsCount;
        }
    }

    // qDebug() << "Found match:" << matchingIp;

    return matchingIp;
}

int countCommonLeadingBitsV6(const Q_IPV6ADDR &addr1, const Q_IPV6ADDR &addr2)
{
    int count = 0;
    for (int i = 0; i < 16; ++i) {
        quint8 diff = addr1[i] ^ addr2[i];
        if (diff == 0) {
            count += 8;
            continue;
        }
        while (!(diff & 0x80)) {
            count++;
            diff <<= 1;
        }
        break;
    }
    return count;
}

static QHostAddress findBestAddressMatchV6(const QVector<QHostAddress> &hostAddresses, const struct sockaddr *fromAddress)
{
    Q_ASSERT(!hostAddresses.empty());

    if (hostAddresses.size() == 1 || fromAddress == nullptr || fromAddress->sa_family != AF_INET6) {
        return hostAddresses[0];
    }

    // QHostAddress(sockaddr *) drops sin6_scope_id, so read it directly
    const sockaddr_in6 *otherSockaddr = reinterpret_cast<const sockaddr_in6 *>(fromAddress);
    const QHostAddress otherIp(otherSockaddr->sin6_addr.s6_addr);

    if (otherIp.isLinkLocal()) {
        // Answer with our link-local address on the interface the query came from
        const QString ifaceName = QNetworkInterface::interfaceNameFromIndex(otherSockaddr->sin6_scope_id);
        for (const QHostAddress &ip : hostAddresses) {
            if (ip.isLinkLocal() && ip.scopeId() == ifaceName) {
                return ip;
            }
        }
        return hostAddresses[0];
    }

    QHostAddress matchingIp = hostAddresses[0];
    int matchingBits = -1;
    const Q_IPV6ADDR rawOtherIp = otherIp.toIPv6Address();
    for (const QHostAddress &ip : hostAddresses) {
        // Same as in findBestAddressMatchV4, compare prefixes since we don't have the network mask
        int matchingBitsCount = countCommonLeadingBitsV6(ip.toIPv6Address(), rawOtherIp);
        if (matchingBitsCount > matchingBits) {
            matchingIp = ip;
            matchingBits = matchingBitsCount;
        }
    }

    return matchingIp;
}

static mdns_record_t createMdnsRecord(const Announcer::AnnouncedInfo &self,
                                      mdns_record_type_t record_type,
                                      const struct sockaddr *fromAddress = nullptr, // used to determine IP to set in A and AAAA records
                                      QHash<QByteArray, QByteArray>::const_iterator txtIterator = {})
{
    mdns_record_t answer;
    answer.type = record_type;
    answer.rclass = 0;
    answer.ttl = 0;
    switch (record_type) {
    case MDNS_RECORDTYPE_PTR: // maps "_<service-type>._tcp.local." to "<instance-name>._<service-type>._tcp.local."
        answer.name = createMdnsString(self.serviceType);
        answer.data.ptr.name = createMdnsString(self.serviceInstance);
        break;
    case MDNS_RECORDTYPE_SRV: // maps "<instance-name>._<service-type>._tcp.local." to "<hostname>.local." and port
        answer.name = createMdnsString(self.serviceInstance);
        answer.data.srv.name = createMdnsString(self.hostname);
        answer.data.srv.port = self.port;
        answer.data.srv.priority = 0;
        answer.data.srv.weight = 0;
        break;
    case MDNS_RECORDTYPE_A: // maps "<hostname>.local." to IPv4
        answer.name = createMdnsString(self.hostname);
        answer.data.a.addr = qHostAddressToSockaddr(findBestAddressMatchV4(self.addressesV4, fromAddress));
        break;
    case MDNS_RECORDTYPE_AAAA: // maps "<hostname>.local." to IPv6
        answer.name = createMdnsString(self.hostname);
        answer.data.aaaa.addr = qHostAddressToSockaddr6(findBestAddressMatchV6(self.addressesV6, fromAddress));
        break;
    case MDNS_RECORDTYPE_TXT:
        answer.name = createMdnsString(self.serviceInstance);
        answer.type = MDNS_RECORDTYPE_TXT;
        answer.data.txt.key = createMdnsString(txtIterator.key());
        answer.data.txt.value = createMdnsString(txtIterator.value());
        break;
    default:
        Q_ASSERT(false);
    }
    return answer;
}

// Callback handling questions incoming on service sockets
static int service_callback(int sock,
                            const struct sockaddr *from,
                            size_t addrlen,
                            mdns_entry_type_t entry_type,
                            uint16_t query_id,
                            uint16_t record_type,
                            uint16_t rclass,
                            uint32_t ttl,
                            const void *data,
                            size_t size,
                            size_t name_offset,
                            size_t name_length,
                            size_t record_offset,
                            size_t record_length,
                            void *user_data)
{
    Q_UNUSED(ttl);
    Q_UNUSED(name_length);
    Q_UNUSED(record_offset);
    Q_UNUSED(record_length);
    static char sendbuffer[2024];

    const Announcer::AnnouncedInfo &self = *((Announcer::AnnouncedInfo *)user_data);

    if (entry_type != MDNS_ENTRYTYPE_QUESTION) {
        return 0;
    }

    static char nameBuffer[256];
    mdns_string_t nameMdnsString = mdns_string_extract(data, size, &name_offset, nameBuffer, sizeof(nameBuffer));
    QByteArray name = QByteArray(nameMdnsString.str, nameMdnsString.length);

    int ret = 0;

    if (name == dnsSdName) {
        if ((record_type == MDNS_RECORDTYPE_PTR) || (record_type == MDNS_RECORDTYPE_ANY)) {
            // The PTR query was for the DNS-SD domain, send answer with a PTR record for the service name we advertise.

            mdns_record_t answer = createMdnsRecord(self, MDNS_RECORDTYPE_PTR);

            uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
            if (unicast) {
                ret = mdns_query_answer_unicast(sock,
                                                from,
                                                addrlen,
                                                sendbuffer,
                                                sizeof(sendbuffer),
                                                query_id,
                                                (mdns_record_type_t)record_type,
                                                nameMdnsString.str,
                                                nameMdnsString.length,
                                                answer,
                                                nullptr,
                                                0,
                                                nullptr,
                                                0);
            } else {
                ret = mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, nullptr, 0, nullptr, 0);
            }
        }
    } else if (name == self.serviceType) {
        if ((record_type == MDNS_RECORDTYPE_PTR) || (record_type == MDNS_RECORDTYPE_ANY)) {
            // The PTR query was for our service, answer a PTR record reverse mapping the queried service name
            // to our service instance name and add additional records containing the SRV record mapping the
            // service instance name to our qualified hostname and port, as well as any IPv4/IPv6 and TXT records

            mdns_record_t answer = createMdnsRecord(self, MDNS_RECORDTYPE_PTR);

            QVector<mdns_record_t> additional;
            additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_SRV));
            if (!self.addressesV4.empty()) {
                additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_A, from));
            }
            if (!self.addressesV6.empty()) {
                additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_AAAA, from));
            }

            for (auto txtIterator = self.txtRecords.cbegin(); txtIterator != self.txtRecords.cend(); txtIterator++) {
                additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_TXT, nullptr, txtIterator));
            }

            uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
            if (unicast) {
                ret = mdns_query_answer_unicast(sock,
                                                from,
                                                addrlen,
                                                sendbuffer,
                                                sizeof(sendbuffer),
                                                query_id,
                                                (mdns_record_type_t)record_type,
                                                nameMdnsString.str,
                                                nameMdnsString.length,
                                                answer,
                                                nullptr,
                                                0,
                                                additional.constData(),
                                                additional.length());
            } else {
                ret = mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, nullptr, 0, additional.constData(), additional.length());
            }
        }
    } else if (name == self.serviceInstance) {
        if ((record_type == MDNS_RECORDTYPE_SRV) || (record_type == MDNS_RECORDTYPE_ANY)) {
            // The SRV query was for our service instance, answer a SRV record mapping the service
            // instance name to our qualified hostname (typically "<hostname>.local.") and port, as
            // well as any IPv4/IPv6 address for the hostname as A/AAAA records and TXT records

            mdns_record_t answer = createMdnsRecord(self, MDNS_RECORDTYPE_SRV);

            QVector<mdns_record_t> additional;
            if (!self.addressesV4.empty()) {
                additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_A, from));
            }
            if (!self.addressesV6.empty()) {
                additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_AAAA, from));
            }

            for (auto txtIterator = self.txtRecords.cbegin(); txtIterator != self.txtRecords.cend(); txtIterator++) {
                additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_TXT, nullptr, txtIterator));
            }

            uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
            if (unicast) {
                ret = mdns_query_answer_unicast(sock,
                                                from,
                                                addrlen,
                                                sendbuffer,
                                                sizeof(sendbuffer),
                                                query_id,
                                                (mdns_record_type_t)record_type,
                                                nameMdnsString.str,
                                                nameMdnsString.length,
                                                answer,
                                                nullptr,
                                                0,
                                                additional.constData(),
                                                additional.length());
            } else {
                ret = mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, nullptr, 0, additional.constData(), additional.length());
            }
        }
    } else if (name == self.hostname) {
        if (((record_type == MDNS_RECORDTYPE_A) || (record_type == MDNS_RECORDTYPE_ANY)) && !self.addressesV4.empty()) {
            // The A query was for our qualified hostname and we have an IPv4 address, answer with an A
            // record mapping the hostname to an IPv4 address, as well as an AAAA record and TXT records

            mdns_record_t answer = createMdnsRecord(self, MDNS_RECORDTYPE_A, from);

            QVector<mdns_record_t> additional;
            if (!self.addressesV6.empty()) {
                additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_AAAA, from));
            }

            for (auto txtIterator = self.txtRecords.cbegin(); txtIterator != self.txtRecords.cend(); txtIterator++) {
                additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_TXT, nullptr, txtIterator));
            }

            uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
            if (unicast) {
                ret = mdns_query_answer_unicast(sock,
                                                from,
                                                addrlen,
                                                sendbuffer,
                                                sizeof(sendbuffer),
                                                query_id,
                                                (mdns_record_type_t)record_type,
                                                nameMdnsString.str,
                                                nameMdnsString.length,
                                                answer,
                                                nullptr,
                                                0,
                                                additional.constData(),
                                                additional.length());
            } else {
                ret = mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, nullptr, 0, additional.constData(), additional.length());
            }
        } else if (((record_type == MDNS_RECORDTYPE_AAAA) || (record_type == MDNS_RECORDTYPE_ANY)) && !self.addressesV6.empty()) {
            // The AAAA query was for our qualified hostname and we have an IPv6 address, answer with an AAAA
            // record mapping the hostname to an IPv4 address, as well as an A record and TXT records

            mdns_record_t answer = createMdnsRecord(self, MDNS_RECORDTYPE_AAAA, from);

            QVector<mdns_record_t> additional;
            if (!self.addressesV4.empty()) {
                additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_A, from));
            }

            for (auto txtIterator = self.txtRecords.cbegin(); txtIterator != self.txtRecords.cend(); txtIterator++) {
                additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_TXT, nullptr, txtIterator));
            }

            uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
            if (unicast) {
                ret = mdns_query_answer_unicast(sock,
                                                from,
                                                addrlen,
                                                sendbuffer,
                                                sizeof(sendbuffer),
                                                query_id,
                                                (mdns_record_type_t)record_type,
                                                nameMdnsString.str,
                                                nameMdnsString.length,
                                                answer,
                                                nullptr,
                                                0,
                                                additional.constData(),
                                                additional.length());
            } else {
                ret = mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, nullptr, 0, additional.constData(), additional.length());
            }
        }
    } // else request is not for me
    if (ret < 0) {
        qCWarning(KDECONNECT_CORE) << "Error sending MDNS query response";
    }
    return ret;
}

// Open sockets to listen to incoming mDNS queries on port 5353
// When receiving, each socket can receive data from all network interfaces
// Thus we only need to open one socket for each address family
int Announcer::listenForQueries()
{
    auto callback = [this](QSocketDescriptor socket) {
        static char buffer[maxReceivedPacketSize];
        MdnsRecords records;
        records.announcer = &self;
        size_t num_records = mdns_socket_listen(socket, buffer, sizeof(buffer), query_callback, &records);

        // Responders may answer queries via multicast instead of unicast (RFC 6762 section 5.4).
        // These reach this socket and not the Discoverer ones, so forward them.
        const bool isResponse = buffer[2] & 0x80;
        if (num_records > 0 && isResponse) {
            const QVector<Discoverer::MdnsService> services = servicesFromRecords(records);
            for (const Discoverer::MdnsService &service : services) {
                Q_EMIT responseReceived(service);
            }
        }
    };

    int numSockets = 0;

    {
        struct sockaddr_in sock_addr;
        memset(&sock_addr, 0, sizeof(struct sockaddr_in));
        sock_addr.sin_family = AF_INET;
#ifdef _WIN32
        sock_addr.sin_addr = in4addr_any;
#else
        sock_addr.sin_addr.s_addr = INADDR_ANY;
#endif
        sock_addr.sin_port = htons(MDNS_PORT);
#ifdef __APPLE__
        sock_addr.sin_len = sizeof(struct sockaddr_in);
#endif
        int socket = mdns_socket_open_ipv4(&sock_addr);
        if (socket >= 0) {
            socketNotifier = new QSocketNotifier(socket, QSocketNotifier::Read);
            QObject::connect(socketNotifier, &QSocketNotifier::activated, this, callback);
            numSockets++;
        }
    }

    {
        struct sockaddr_in6 sock_addr;
        memset(&sock_addr, 0, sizeof(struct sockaddr_in6));
        sock_addr.sin6_family = AF_INET6;
        sock_addr.sin6_addr = in6addr_any;
        sock_addr.sin6_port = htons(MDNS_PORT);
#ifdef __APPLE__
        sock_addr.sin6_len = sizeof(struct sockaddr_in6);
#endif
        int socket = mdns_socket_open_ipv6(&sock_addr);
        if (socket >= 0) {
            socketNotifierV6 = new QSocketNotifier(socket, QSocketNotifier::Read);
            QObject::connect(socketNotifierV6, &QSocketNotifier::activated, this, callback);
            numSockets++;
        }
    }

    return numSockets;
}

Announcer::Announcer(const QString &instanceName, const QString &serviceType, uint16_t port)
{
    self.serviceType = serviceType.toLatin1();
    if (!self.serviceType.endsWith('.')) {
        // mdns.h needs all the qualified names to end with dot for some reason
        self.serviceType.append('.');
    }
    self.serviceInstance = instanceName.toLatin1() + '.' + self.serviceType;
    self.hostname = QHostInfo::localHostName().toLatin1();
    self.hostname.append(self.hostname.endsWith(".local") ? "." : ".local.");
    self.port = port;
    detectHostAddresses();
}

// Heuristic to detect virtual interfaces (bridges, VM/container adapters, etc.) whose addresses
// are typically not reachable from other devices on the physical LAN. Advertising such addresses
// in our mDNS A/AAAA records can make our hostname resolve to an unroutable address on remote
// devices. We can't rely on QNetworkInterface::type(): bridges like virbr0/docker0 report as
// Ethernet (ARPHRD_ETHER), while the only things flagged Virtual are tunnels (tun/WireGuard),
// which we want to keep advertising in case a VPN is the machine's only connectivity. So we match
// on well-known virtual-interface name prefixes instead.
static bool isVirtualInterface(const QNetworkInterface &iface)
{
    // Only clearly-virtual interfaces that are never a machine's primary LAN link. We deliberately
    // avoid prefixes that can legitimately be the real connection (e.g. "bridge" on a virtualization
    // host, or "tun"/"tap" when a VPN is the only connectivity), since wrongly dropping those would
    // leave us advertising no usable address at all.
    static const QStringList virtualPrefixes = {
        QStringLiteral("virbr"), // libvirt bridge
        QStringLiteral("vnet"), // libvirt guest tap
        QStringLiteral("vmnet"), // VMware host-only/NAT
        QStringLiteral("vboxnet"), // VirtualBox host-only
        QStringLiteral("docker"), // Docker bridge
        QStringLiteral("br-"), // Docker user-defined bridges
        QStringLiteral("veth"), // container veth pairs
        QStringLiteral("awdl"), // macOS Apple Wireless Direct Link (AirDrop, not LAN)
        QStringLiteral("llw"), // macOS low-latency WLAN (peer-to-peer, not LAN)
    };

    const QString name = iface.name();
    for (const QString &prefix : virtualPrefixes) {
        if (name.startsWith(prefix)) {
            return true;
        }
    }
    return false;
}

void Announcer::detectHostAddresses()
{
    self.addressesV4.clear();
    self.addressesV6.clear();
    addressesByInterface.clear();
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        int flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::CanMulticast) || (flags & QNetworkInterface::IsLoopBack)) {
            continue;
        }
        if (isVirtualInterface(iface)) {
            qCDebug(KDECONNECT_CORE) << "Skipping virtual interface for mDNS announce:" << iface.name();
            continue;
        }
        for (const QNetworkAddressEntry &ifaceAddress : iface.addressEntries()) {
            QHostAddress sourceAddress = ifaceAddress.ip();
            if (sourceAddress.protocol() == QAbstractSocket::IPv4Protocol && sourceAddress != QHostAddress::LocalHost) {
                self.addressesV4.append(sourceAddress);
                addressesByInterface[iface.index()].append(sourceAddress);
            } else if (sourceAddress.protocol() == QAbstractSocket::IPv6Protocol && sourceAddress != QHostAddress::LocalHostIPv6) {
                self.addressesV6.append(sourceAddress);
                addressesByInterface[iface.index()].append(sourceAddress);
            }
        }
    }
}

void Announcer::startAnnouncing()
{
    int num_sockets = listenForQueries();
    if (num_sockets <= 0) {
        qCWarning(KDECONNECT_CORE) << "Failed to open any MDNS client sockets";
        return;
    }
    sendMulticastAnnounce(false);
}

void Announcer::stopAnnouncing()
{
    sendMulticastAnnounce(true);
    stopListeningForQueries();
}

void Announcer::stopListeningForQueries()
{
    if (socketNotifier) {
        mdns_socket_close(socketNotifier->socket());
        delete socketNotifier;
        socketNotifier = nullptr;
    }
    if (socketNotifierV6) {
        mdns_socket_close(socketNotifierV6->socket());
        delete socketNotifierV6;
        socketNotifierV6 = nullptr;
    }
}

static void sendAnnouncement(int socket, const Announcer::AnnouncedInfo &self, bool isGoodbye)
{
    mdns_record_t ptr_record = createMdnsRecord(self, MDNS_RECORDTYPE_PTR);

    QVector<mdns_record_t> additional;
    additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_SRV));
    for (auto txtIterator = self.txtRecords.cbegin(); txtIterator != self.txtRecords.cend(); txtIterator++) {
        additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_TXT, nullptr, txtIterator));
    }

    for (const QHostAddress &address : self.addressesV4) {
        mdns_record_t record = createMdnsRecord(self, MDNS_RECORDTYPE_A);
        record.data.a.addr = qHostAddressToSockaddr(address);
        additional.append(record);
    }
    for (const QHostAddress &address : self.addressesV6) {
        mdns_record_t record = createMdnsRecord(self, MDNS_RECORDTYPE_AAAA);
        record.data.aaaa.addr = qHostAddressToSockaddr6(address);
        additional.append(record);
    }

    char buffer[2048];
    const auto sendMulticast = isGoodbye ? mdns_goodbye_multicast : mdns_announce_multicast;
    if (sendMulticast(socket, buffer, sizeof(buffer), ptr_record, nullptr, 0, additional.constData(), additional.size()) < 0) {
        qCWarning(KDECONNECT_CORE) << "Failed to send mDNS" << (isGoodbye ? "goodbye" : "announcement");
    }
}

void Announcer::sendMulticastAnnounce(bool isGoodbye)
{
    qCDebug(KDECONNECT_CORE) << "Sending mDNS" << (isGoodbye ? "goodbye" : "announcement");
    for (auto it = addressesByInterface.cbegin(); it != addressesByInterface.cend(); ++it) {
        AnnouncedInfo info = self;
        info.addressesV4.clear();
        info.addressesV6.clear();
        for (const QHostAddress &address : it.value()) {
            if (address.protocol() == QAbstractSocket::IPv4Protocol) {
                info.addressesV4.append(address);
            } else {
                info.addressesV6.append(address);
            }
        }

        auto sendOnInterface = [&](QSocketNotifier *notifier, int level, int option, const auto &outgoingInterface) {
            if (!notifier) {
                return;
            }
            const int socket = notifier->socket();
            auto previousInterface = outgoingInterface;
            socklen_t length = sizeof(previousInterface);
            if (getsockopt(socket, level, option, reinterpret_cast<char *>(&previousInterface), &length) < 0
                || setsockopt(socket, level, option, reinterpret_cast<const char *>(&outgoingInterface), sizeof(outgoingInterface)) < 0) {
                qCWarning(KDECONNECT_CORE) << "Failed to select mDNS multicast interface" << it.key();
                return;
            }
            sendAnnouncement(socket, info, isGoodbye);
            if (setsockopt(socket, level, option, reinterpret_cast<const char *>(&previousInterface), sizeof(previousInterface)) < 0) {
                qCWarning(KDECONNECT_CORE) << "Failed to restore mDNS multicast interface";
            }
        };

        if (!info.addressesV4.isEmpty()) {
            const in_addr address = qHostAddressToSockaddr(info.addressesV4.first()).sin_addr;
            sendOnInterface(socketNotifier, IPPROTO_IP, IP_MULTICAST_IF, address);
        }
        if (!info.addressesV6.isEmpty()) {
            sendOnInterface(socketNotifierV6, IPPROTO_IPV6, IPV6_MULTICAST_IF, it.key());
        }
    }
}

} // namespace MdnshWrapper
