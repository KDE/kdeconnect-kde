/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mdns_wrapper.h"

#include "core_debug.h"

#include "mdns.h"

#include <errno.h>

#include <QHostInfo>
#include <QNetworkInterface>
#include <QSocketNotifier>

namespace MdnsWrapper
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

// Callback that handles responses to a query
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
    Q_UNUSED(sock);
    Q_UNUSED(addrlen);
    Q_UNUSED(query_id);
    Q_UNUSED(entry_type);
    Q_UNUSED(rclass);
    Q_UNUSED(ttl);
    Q_UNUSED(name_offset);
    Q_UNUSED(name_length);

    // qCDebug(KDECONNECT_CORE) << "Received DNS record of type" << recordTypeToStr(record_type) << "from socket" << sock << "with type" <<
    // entryTypeToStr(entry_type);

    Discoverer::MdnsService *discoveredService = (Discoverer::MdnsService *)user_data;

    switch (record_type) {
    case MDNS_RECORDTYPE_PTR: {
        // We don't use mdns_record_parse_ptr() because we want to extract just the service name instead of the full
        // "<instance-name>._<service-type>._tcp.local." string
        mdns_string_pair_t instanceNamePos = mdns_get_next_substring(data, size, record_offset);
        discoveredService->name = QString::fromLatin1((char *)data + instanceNamePos.offset, instanceNamePos.length);
        // static char instanceNameBuffer[256];
        // mdns_string_t instanceName = mdns_record_parse_ptr(data, size, record_offset, record_length, instanceNameBuffer, sizeof(instanceNameBuffer));
        // discoveredService->name = QString::fromLatin1(instanceName.str, instanceName.length);
        if (discoveredService->address == QHostAddress::Null) {
            discoveredService->address = QHostAddress(from); // In case we don't receive a A record, use from as address
        }
    } break;
    case MDNS_RECORDTYPE_SRV: {
        static char nameBuffer[256];
        mdns_record_srv_t record = mdns_record_parse_srv(data, size, record_offset, record_length, nameBuffer, sizeof(nameBuffer));
        // We can use the IP to connect so we don't need to store the "<hostname>.local." address.
        // discoveredService->qualifiedHostname = QString::fromLatin1(record.name.str, record.name.length);
        discoveredService->port = record.port;
    } break;
    case MDNS_RECORDTYPE_A: {
        sockaddr_in addr;
        mdns_record_parse_a(data, size, record_offset, record_length, &addr);
        discoveredService->address = QHostAddress(ntohl(addr.sin_addr.s_addr));
    } break;
    case MDNS_RECORDTYPE_AAAA:
        // Ignore IPv6 for now
        // sockaddr_in6 addr6;
        // mdns_record_parse_aaaa(data, size, record_offset, record_length, &addr6);
        break;
    case MDNS_RECORDTYPE_TXT: {
        mdns_record_txt_t records[24];
        size_t parsed = mdns_record_parse_txt(data, size, record_offset, record_length, records, sizeof(records) / sizeof(mdns_record_txt_t));
        for (size_t itxt = 0; itxt < parsed; ++itxt) {
            QString key = QString::fromLatin1(records[itxt].key.str, records[itxt].key.length);
            QString value = QString::fromLatin1(records[itxt].value.str, records[itxt].value.length);
            discoveredService->txtRecords[key] = value;
        }
    } break;
    default:
        // Ignore unknown record types
        break;
    }

    return 0;
}

void Discoverer::startDiscovering(const QString &serviceType)
{
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
    // Open a socket for each interface
    QVector<int> sockets;
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
                    sockets.append(socket);
                } else {
                    qCDebug(KDECONNECT_CORE) << "Couldn't open socket";
                }
            } else if (sourceAddress.protocol() == QAbstractSocket::IPv6Protocol && sourceAddress != QHostAddress::LocalHostIPv6) {
                qCDebug(KDECONNECT_CORE) << "Opening socket for address6" << sourceAddress;
                struct sockaddr_in6 saddr = qHostAddressToSockaddr6(sourceAddress);
                int socket = mdns_socket_open_ipv6(&saddr);
                if (socket >= 0) {
                    sockets.append(socket);
                } else {
                    qCDebug(KDECONNECT_CORE) << "Couldn't open socket";
                }
            }
        }
    }

    // Start listening on all sockets
    for (int socket : std::as_const(sockets)) {
        QSocketNotifier *socketNotifier = new QSocketNotifier(socket, QSocketNotifier::Read);
        QObject::connect(socketNotifier, &QSocketNotifier::activated, [this](QSocketDescriptor socket) {
            MdnsService discoveredService;

            static char buffer[2048];
            size_t num_records = mdns_query_recv(socket, buffer, sizeof(buffer), query_callback, (void *)&discoveredService, 0);
            Q_UNUSED(num_records);

            // qCDebug(KDECONNECT_CORE) << "Discovered service" << discoveredService.name << "at" << discoveredService.address << "in" <<  num_records <<
            // "records via socket" << socket;

            Q_EMIT serviceFound(discoveredService);
        });
        responseSocketNotifiers.append(socketNotifier);
    }

    qCDebug(KDECONNECT_CORE) << "Opened" << sockets.size() << "sockets to listen for MDNS query responses";

    return sockets.size();
}

void Discoverer::sendQuery(const QString &serviceType)
{
    qCDebug(KDECONNECT_CORE) << "Sending MDNS query for service" << serviceType;

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

static QHostAddress findBestAddressMatchV6(const QVector<QHostAddress> &hostAddresses, const struct sockaddr *fromAddress)
{
    Q_ASSERT(!hostAddresses.empty());
    // We could do the same logic for v6 that we do for V4, but we don't care that much about IPv6
    return hostAddresses[0];
    Q_UNUSED(fromAddress);
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
// When recieving, each socket can recieve data from all network interfaces
// Thus we only need to open one socket for each address family
int Announcer::listenForQueries()
{
    auto callback = [this](QSocketDescriptor socket) {
        static char buffer[2048];
        mdns_socket_listen(socket, buffer, sizeof(buffer), service_callback, &self);
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
            QObject::connect(socketNotifier, &QSocketNotifier::activated, callback);
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
            QObject::connect(socketNotifierV6, &QSocketNotifier::activated, callback);
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
    self.hostname = QHostInfo::localHostName().toLatin1() + ".local.";
    self.port = port;
    detectHostAddresses();
}

void Announcer::detectHostAddresses()
{
    self.addressesV4.clear();
    self.addressesV6.clear();
    for (const QNetworkInterface &iface : QNetworkInterface::allInterfaces()) {
        int flags = iface.flags();
        if (!(flags & QNetworkInterface::IsUp) || !(flags & QNetworkInterface::CanMulticast) || (flags & QNetworkInterface::IsLoopBack)) {
            continue;
        }
        for (const QNetworkAddressEntry &ifaceAddress : iface.addressEntries()) {
            QHostAddress sourceAddress = ifaceAddress.ip();
            if (sourceAddress.protocol() == QAbstractSocket::IPv4Protocol && sourceAddress != QHostAddress::LocalHost) {
                self.addressesV4.append(sourceAddress);
            } else if (sourceAddress.protocol() == QAbstractSocket::IPv6Protocol && sourceAddress != QHostAddress::LocalHostIPv6) {
                self.addressesV6.append(sourceAddress);
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
    delete socketNotifier;
    socketNotifier = nullptr;
    delete socketNotifierV6;
    socketNotifierV6 = nullptr;
}

void Announcer::sendMulticastAnnounce(bool isGoodbye)
{
    mdns_record_t ptr_record = createMdnsRecord(self, MDNS_RECORDTYPE_PTR);

    QVector<mdns_record_t> additional;
    additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_SRV));
    if (!self.addressesV4.empty()) {
        additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_A));
    }
    if (!self.addressesV6.empty()) {
        additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_AAAA));
    }

    for (auto txtIterator = self.txtRecords.cbegin(); txtIterator != self.txtRecords.cend(); txtIterator++) {
        additional.append(createMdnsRecord(self, MDNS_RECORDTYPE_TXT, nullptr, txtIterator));
    }

    static char buffer[2048];
    if (isGoodbye) {
        qCDebug(KDECONNECT_CORE) << "Sending goodbye";
        if (socketNotifier)
            mdns_goodbye_multicast(socketNotifier->socket(), buffer, sizeof(buffer), ptr_record, nullptr, 0, additional.constData(), additional.length());
        if (socketNotifierV6)
            mdns_goodbye_multicast(socketNotifierV6->socket(), buffer, sizeof(buffer), ptr_record, nullptr, 0, additional.constData(), additional.length());
    } else {
        qCDebug(KDECONNECT_CORE) << "Sending announce";
        if (socketNotifier)
            mdns_announce_multicast(socketNotifier->socket(), buffer, sizeof(buffer), ptr_record, nullptr, 0, additional.constData(), additional.length());
        if (socketNotifierV6)
            mdns_announce_multicast(socketNotifierV6->socket(), buffer, sizeof(buffer), ptr_record, nullptr, 0, additional.constData(), additional.length());
    }
}

} // namespace MdnsWrapper
