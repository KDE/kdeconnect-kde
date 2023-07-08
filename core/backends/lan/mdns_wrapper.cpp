/**
 * SPDX-FileCopyrightText: 2023 Albert Vaca Cintora <albertvaka@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
 */

#include "mdns_wrapper.h"

#include "core_debug.h"

#include "mdns.h"

#include <errno.h>
#include <signal.h>

#ifdef _WIN32
#include <iphlpapi.h>
#include <winsock2.h>
#else
#include <ifaddrs.h>
#include <net/if.h>
#include <netdb.h>
#include <sys/time.h>
#endif

#include <QHostInfo>
#include <QNetworkInterface>
#include <QSocketNotifier>

const char *recordTypeToStr(int rtype)
{
    switch (rtype) {
        case MDNS_RECORDTYPE_PTR: return "PTR";
        case MDNS_RECORDTYPE_SRV: return "SRV";
        case MDNS_RECORDTYPE_TXT: return "TXT";
        case MDNS_RECORDTYPE_A: return "A";
        case MDNS_RECORDTYPE_AAAA: return "AAAA";
        case MDNS_RECORDTYPE_ANY: return "ANY";
        default: return "UNKNOWN";
    }
}

const char *entryTypeToStr(int entry)
{
    switch (entry) {
        case MDNS_ENTRYTYPE_QUESTION: return "QUESTION";
        case MDNS_ENTRYTYPE_ANSWER: return "ANSWER";
        case MDNS_ENTRYTYPE_AUTHORITY: return "AUTHORITY";
        case MDNS_ENTRYTYPE_ADDITIONAL: return "ADDITIONAL";
        default: return "UNKNOWN";
    }
}

// Callback that handles responses to a query
static int query_callback(int sock, const struct sockaddr* from, size_t addrlen, mdns_entry_type_t entry_type,
               uint16_t query_id, uint16_t record_type, uint16_t rclass, uint32_t ttl, const void* data,
               size_t size, size_t name_offset, size_t name_length, size_t record_offset,
               size_t record_length, void* user_data) {
    Q_UNUSED(sock);
    Q_UNUSED(addrlen);
    Q_UNUSED(query_id);
    Q_UNUSED(entry_type);
    Q_UNUSED(rclass);
    Q_UNUSED(ttl);
    Q_UNUSED(name_offset);
    Q_UNUSED(name_length);

    //qCDebug(KDECONNECT_CORE) << "Received DNS record of type" << recordTypeToStr(record_type) << "from socket" << sock << "with type" << entryTypeToStr(entry_type);

    MdnsWrapper::MdnsService *discoveredService = (MdnsWrapper::MdnsService *)user_data;

    switch (record_type) {
    case MDNS_RECORDTYPE_PTR: {
        // Keep just the service name instead of the full "<service-name>.<_service-type>._tcp.local." string
        mdns_string_pair_t serviceNamePos = mdns_get_next_substring(data, size, record_offset);
        discoveredService->name = QString::fromLatin1((char *)data + serviceNamePos.offset, serviceNamePos.length);
        //static char serviceNameBuffer[256];
        //mdns_string_t serviceName = mdns_record_parse_ptr(data, size, record_offset, record_length, serviceNameBuffer, sizeof(serviceNameBuffer));
        //discoveredService->name = QString::fromLatin1(serviceName.str, serviceName.length);
        if (discoveredService->address == QHostAddress::Null) {
            discoveredService->address = QHostAddress(from); // In case we don't receive a A record, use from as address
        }
    } break;
    case MDNS_RECORDTYPE_SRV: {
        static char nameBuffer[256];
        mdns_record_srv_t record = mdns_record_parse_srv(data, size, record_offset, record_length, nameBuffer, sizeof(nameBuffer));
        // We can use the IP to connect so we don't need to store the "<hostname>.local." address.
        //discoveredService->qualifiedHostname = QString::fromLatin1(record.name.str, record.name.length);
        discoveredService->port = record.port;
    } break;
    case MDNS_RECORDTYPE_A: {
        sockaddr_in addr;
        mdns_record_parse_a(data, size, record_offset, record_length, &addr);
        discoveredService->address = QHostAddress(ntohl(addr.sin_addr.s_addr));
    } break;
    case MDNS_RECORDTYPE_AAAA:
        // Ignore IPv6 for now
        //sockaddr_in6 addr6;
        //mdns_record_parse_aaaa(data, size, record_offset, record_length, &addr6);
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

void MdnsWrapper::startDiscovering(const QString &serviceType)
{
    int num_sockets = listenForQueryResponses();
    if (num_sockets <= 0) {
        qWarning() << "Failed to open any client sockets";
        return;
    }
    sendQuery(serviceType);
}

void MdnsWrapper::stopDiscovering()
{
    stopListeningForQueryResponses();
}

void MdnsWrapper::stopListeningForQueryResponses()
{
    qCDebug(KDECONNECT_CORE) << "Closing" << responseSocketNotifiers.size() << "sockets";
    for (QSocketNotifier *socketNotifier : responseSocketNotifiers) {
        mdns_socket_close(socketNotifier->socket());
        delete socketNotifier;
    }
    responseSocketNotifiers.clear();
}

int MdnsWrapper::listenForQueryResponses()
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
                struct sockaddr_in saddr;
                memset(&saddr, 0, sizeof(saddr));
                saddr.sin_family = AF_INET;
                saddr.sin_port = 0;
                saddr.sin_addr.s_addr = htonl(sourceAddress.toIPv4Address());
                int socket = mdns_socket_open_ipv4(&saddr);
                sockets.append(socket);
            }
            // Ignore IPv6 interfaces for now
        }
    }

    // Start listening on all sockets
    for (int socket : sockets) {
        QSocketNotifier *socketNotifier = new QSocketNotifier(socket, QSocketNotifier::Read);
        QObject::connect(socketNotifier, &QSocketNotifier::activated, [this](QSocketDescriptor socket) {
            MdnsService discoveredService;

            size_t capacity = 2048;
            void *buffer = malloc(capacity);
            size_t num_records = mdns_query_recv(socket, buffer, capacity, query_callback, (void *)&discoveredService, 0);
            free(buffer);

            // qCDebug(KDECONNECT_CORE) << "Discovered service" << discoveredService.name << "at" << discoveredService.address << "in" <<  num_records <<
            // "records via socket" << socket;

            Q_EMIT serviceFound(discoveredService);
        });
        responseSocketNotifiers.append(socketNotifier);
    }

    qCDebug(KDECONNECT_CORE) << "Opened" << sockets.size() << "sockets to listen for MDNS query responses";

    return sockets.size();
}

void MdnsWrapper::sendQuery(const QString &serviceType)
{
    qCDebug(KDECONNECT_CORE) << "Sending MDNS query for service" << serviceType;

    mdns_query_t query;
    QByteArray serviceTypeBytes = serviceType.toLatin1();
    query.name = serviceTypeBytes.data();
    query.length = serviceTypeBytes.length();
    query.type = MDNS_RECORDTYPE_PTR;

    size_t capacity = 2048;
    void *buffer = malloc(capacity);
    for (QSocketNotifier *socketNotifier : responseSocketNotifiers) {
        int socket = socketNotifier->socket();
        qCDebug(KDECONNECT_CORE) << "Sending mDNS query via socket" << socket;
        int ret = mdns_multiquery_send(socket, &query, 1, buffer, capacity, 0);
        if (ret < 0) {
            qWarning() << "Failed to send mDNS query:" << strerror(errno);
        }
    }
    free(buffer);
}











































// Those are the IPs we will announce. They are the first IP we find. Maybe we should announce all our IPs instead?
struct sockaddr_in service_address_ipv4;
struct sockaddr_in6 service_address_ipv6;


static mdns_string_t ipv4_address_to_string(char* buffer, size_t capacity, const struct sockaddr_in* addr,
                       size_t addrlen) {
    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    int ret = getnameinfo((const struct sockaddr*)addr, (socklen_t)addrlen, host, NI_MAXHOST,
                          service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
    int len = 0;
    if (ret == 0) {
        if (addr->sin_port != 0)
            len = snprintf(buffer, capacity, "%s:%s", host, service);
        else
            len = snprintf(buffer, capacity, "%s", host);
    }
    if (len >= (int)capacity)
        len = (int)capacity - 1;
    mdns_string_t str;
    str.str = buffer;
    str.length = len;
    return str;
}

static mdns_string_t ipv6_address_to_string(char* buffer, size_t capacity, const struct sockaddr_in6* addr,
                       size_t addrlen) {
    char host[NI_MAXHOST] = {0};
    char service[NI_MAXSERV] = {0};
    int ret = getnameinfo((const struct sockaddr*)addr, (socklen_t)addrlen, host, NI_MAXHOST,
                          service, NI_MAXSERV, NI_NUMERICSERV | NI_NUMERICHOST);
    int len = 0;
    if (ret == 0) {
        if (addr->sin6_port != 0)
            len = snprintf(buffer, capacity, "[%s]:%s", host, service);
        else
            len = snprintf(buffer, capacity, "%s", host);
    }
    if (len >= (int)capacity)
        len = (int)capacity - 1;
    mdns_string_t str;
    str.str = buffer;
    str.length = len;
    return str;
}

static mdns_string_t ip_address_to_string(char* buffer, size_t capacity, const struct sockaddr* addr, size_t addrlen) {
    if (addr->sa_family == AF_INET6)
        return ipv6_address_to_string(buffer, capacity, (const struct sockaddr_in6*)addr, addrlen);
    return ipv4_address_to_string(buffer, capacity, (const struct sockaddr_in*)addr, addrlen);
}



char service_instance_buffer[256] = {0};
char qualified_hostname_buffer[256] = {0};

static char sendbuffer[1024];
static mdns_record_txt_t txtbuffer[128];
static char addrbuffer[64];
static char entrybuffer[256];

const QString dnsSdName = QStringLiteral("_services._dns-sd._udp.local.");

// Callback handling questions incoming on service sockets
static int service_callback(int sock, const struct sockaddr* from, size_t addrlen, mdns_entry_type_t entry_type,
                 uint16_t query_id, uint16_t record_type, uint16_t rclass, uint32_t ttl, const void* data,
                 size_t size, size_t name_offset, size_t name_length, size_t record_offset,
                 size_t record_length, void* user_data) {
    Q_UNUSED(ttl);
    Q_UNUSED(name_length);
    Q_UNUSED(record_offset);
    Q_UNUSED(record_length);

    MdnsServiceAnnouncer::AnnouncedInfo *myself = (MdnsServiceAnnouncer::AnnouncedInfo *)user_data;

    if (entry_type != MDNS_ENTRYTYPE_QUESTION) {
        return 0;
    }

    static char nameBuffer[256];
    mdns_string_t nameMdnsString = mdns_string_extract(data, size, &name_offset, nameBuffer, sizeof(nameBuffer));
    QString name = QString::fromLatin1(nameMdnsString.str, nameMdnsString.length);

    if (name == dnsSdName) {
        qWarning() << "Someone queried all services for" << recordTypeToStr(record_type);
        if ((record_type == MDNS_RECORDTYPE_PTR) || (record_type == MDNS_RECORDTYPE_ANY)) {
            // The PTR query was for the DNS-SD domain, send answer with a PTR record for the
            // service name we advertise, typically on the "<_service-name>._tcp.local." format

            /*
            // Answer PTR record reverse mapping "<_service-name>._tcp.local." to "<hostname>.<_service-name>._tcp.local."
            mdns_record_t answer;
            answer.name = nameMdnsString;
            answer.type = MDNS_RECORDTYPE_PTR;
            answer.data.ptr.name = service.service;

            // Send the answer, unicast or multicast depending on flag in query
            uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
            printf("  --> answer %.*s (%s)\n", MDNS_STRING_FORMAT(answer.data.ptr.name),
                   (unicast ? "unicast" : "multicast"));

            if (unicast) {
                mdns_query_answer_unicast(sock, (void*)from, addrlen, (void*)sendbuffer, sizeof(sendbuffer),
                                          query_id, (mdns_record_type_t)record_type, nameMdnsString.str, nameMdnsString.length, answer, NULL, 0, NULL, 0);
            } else {
                mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0, 0, 0);
            }
            */
        }
    } else if (name == myself->serviceType) {
        qWarning() << "Someone queried my service type for" << recordTypeToStr(record_type);
        if ((record_type == MDNS_RECORDTYPE_PTR) || (record_type == MDNS_RECORDTYPE_ANY)) {
            // The PTR query was for our service (usually "<_service-name._tcp.local"), answer a PTR
            // record reverse mapping the queried service name to our service instance name
            // (typically on the "<hostname>.<_service-name>._tcp.local." format), and add
            // additional records containing the SRV record mapping the service instance name to our
            // qualified hostname (typically "<hostname>.local.") and port, as well as any IPv4/IPv6
            // address for the hostname as A/AAAA records, and two test TXT records

            /*
            // Answer PTR record reverse mapping "<_service-name>._tcp.local." to
            // "<hostname>.<_service-name>._tcp.local."
            mdns_record_t answer = service.record_ptr;

            mdns_record_t additional[5] = {0};
            size_t additional_count = 0;

            // SRV record mapping "<hostname>.<_service-name>._tcp.local." to
            // "<hostname>.local." with port. Set weight & priority to 0.
            additional[additional_count++] = service.record_srv;

            // A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
            if (service.address_ipv4.sin_family == AF_INET)
                additional[additional_count++] = service.record_a;
            if (service.address_ipv6.sin6_family == AF_INET6)
                additional[additional_count++] = service.record_aaaa;

            // Add two test TXT records for our service instance name, will be coalesced into
            // one record with both key-value pair strings by the library
            additional[additional_count++] = service.txt_record[0];
            additional[additional_count++] = service.txt_record[1];

            // Send the answer, unicast or multicast depending on flag in query
            uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
            printf("  --> answer %.*s (%s)\n",
                   MDNS_STRING_FORMAT(service.record_ptr.data.ptr.name),
                   (unicast ? "unicast" : "multicast"));

            if (unicast) {
                mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer),
                                          query_id, (mdns_record_type_t)record_type, name.str, name.length, answer, 0, 0,
                                          additional, additional_count);
            } else {
                mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0,
                                            additional, additional_count);
            }
            */
        }
    } else if (name == myself->serviceInstance) {
        qWarning() << "Someone queried my service instance" << recordTypeToStr(record_type);
        if ((record_type == MDNS_RECORDTYPE_SRV) || (record_type == MDNS_RECORDTYPE_ANY)) {
            // The SRV query was for our service instance (usually
            // "<hostname>.<_service-name._tcp.local"), answer a SRV record mapping the service
            // instance name to our qualified hostname (typically "<hostname>.local.") and port, as
            // well as any IPv4/IPv6 address for the hostname as A/AAAA records, and two test TXT
            // records

            /*
            // Answer PTR record reverse mapping "<_service-name>._tcp.local." to
            // "<hostname>.<_service-name>._tcp.local."
            mdns_record_t answer = service.record_srv;

            mdns_record_t additional[5] = {0};
            size_t additional_count = 0;

            // A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
            if (service.address_ipv4.sin_family == AF_INET)
                additional[additional_count++] = service.record_a;
            if (service.address_ipv6.sin6_family == AF_INET6)
                additional[additional_count++] = service.record_aaaa;

            // Add two test TXT records for our service instance name, will be coalesced into
            // one record with both key-value pair strings by the library
            additional[additional_count++] = service.txt_record[0];
            additional[additional_count++] = service.txt_record[1];

            // Send the answer, unicast or multicast depending on flag in query
            uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
            printf("  --> answer %.*s port %d (%s)\n",
                   MDNS_STRING_FORMAT(service.record_srv.data.srv.name), service.port,
                   (unicast ? "unicast" : "multicast"));

            if (unicast) {
                mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer),
                                          query_id, (mdns_record_type_t)record_type, name.str, name.length, answer, 0, 0,
                                          additional, additional_count);
            } else {
                mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0,
                                            additional, additional_count);
            }

            */
        }
    } else if (name == myself->hostname) {
        qWarning() << "Someone queried my host for" << recordTypeToStr(record_type);
        if (((record_type == MDNS_RECORDTYPE_A) || (record_type == MDNS_RECORDTYPE_ANY)) /* && socketNotifier */) {
            // The A query was for our qualified hostname (typically "<hostname>.local.") and we
            // have an IPv4 address, answer with an A record mappiing the hostname to an IPv4
            // address, as well as any IPv6 address for the hostname, and two test TXT records

            /*
            // Answer A records mapping "<hostname>.local." to IPv4 address
            mdns_record_t answer = service.record_a;

            mdns_record_t additional[5] = {0};
            size_t additional_count = 0;

            // AAAA record mapping "<hostname>.local." to IPv6 addresses
            if (service.address_ipv6.sin6_family == AF_INET6)
                additional[additional_count++] = service.record_aaaa;

            // Add two test TXT records for our service instance name, will be coalesced into
            // one record with both key-value pair strings by the library
            additional[additional_count++] = service.txt_record[0];
            additional[additional_count++] = service.txt_record[1];

            // Send the answer, unicast or multicast depending on flag in query
            uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
            mdns_string_t addrstr = ip_address_to_string(
                addrbuffer, sizeof(addrbuffer), (struct sockaddr*)&service.record_a.data.a.addr,
                sizeof(service.record_a.data.a.addr));
            printf("  --> answer %.*s IPv4 %.*s (%s)\n", MDNS_STRING_FORMAT(service.record_a.name),
                MDNS_STRING_FORMAT(addrstr), (unicast ? "unicast" : "multicast"));

            if (unicast) {
                mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer),
                                        query_id, (mdns_record_type_t)record_type, name.str, name.length, answer, 0, 0,
                                        additional, additional_count);
            } else {
                mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0,
                                            additional, additional_count);
            }
            */
        } else if (((record_type == MDNS_RECORDTYPE_AAAA) || (record_type == MDNS_RECORDTYPE_ANY)) /* && socketNotifierV6 */) {
            // The AAAA query was for our qualified hostname (typically "<hostname>.local.") and we
            // have an IPv6 address, answer with an AAAA record mappiing the hostname to an IPv6
            // address, as well as any IPv4 address for the hostname, and two test TXT records

            /*
            // Answer AAAA records mapping "<hostname>.local." to IPv6 address
            mdns_record_t answer = service.record_aaaa;

            mdns_record_t additional[5] = {0};
            size_t additional_count = 0;

            // A record mapping "<hostname>.local." to IPv4 addresses
            if (service.address_ipv4.sin_family == AF_INET)
                additional[additional_count++] = service.record_a;

            // Add two test TXT records for our service instance name, will be coalesced into
            // one record with both key-value pair strings by the library
            additional[additional_count++] = service.txt_record[0];
            additional[additional_count++] = service.txt_record[1];

            // Send the answer, unicast or multicast depending on flag in query
            uint16_t unicast = (rclass & MDNS_UNICAST_RESPONSE);
            mdns_string_t addrstr =
                ip_address_to_string(addrbuffer, sizeof(addrbuffer),
                                    (struct sockaddr*)&service.record_aaaa.data.aaaa.addr,
                                    sizeof(service.record_aaaa.data.aaaa.addr));
            printf("  --> answer %.*s IPv6 %.*s (%s)\n",
                MDNS_STRING_FORMAT(service.record_aaaa.name), MDNS_STRING_FORMAT(addrstr),
                (unicast ? "unicast" : "multicast"));

            if (unicast) {
                mdns_query_answer_unicast(sock, from, addrlen, sendbuffer, sizeof(sendbuffer),
                                        query_id, (mdns_record_type_t)record_type, name.str, name.length, answer, 0, 0,
                                        additional, additional_count);
            } else {
                mdns_query_answer_multicast(sock, sendbuffer, sizeof(sendbuffer), answer, 0, 0,
                                            additional, additional_count);
            }
            */
        }
    } // else request is not for me
    return 0;
}



// Open sockets to listen to incoming mDNS queries on port 5353
// When recieving, each socket can recieve data from all network interfaces
// Thus we only need to open one socket for each address family
int MdnsServiceAnnouncer::listenForQueries()
{
    auto callback = [this](QSocketDescriptor socket) {
        size_t capacity = 2048;
        void *buffer = malloc(capacity);
        mdns_socket_listen(socket, buffer, capacity, service_callback, &myself);
        free(buffer);
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

MdnsServiceAnnouncer::MdnsServiceAnnouncer(const QString &name, const QString &serviceType, int port)
{
    myself.serviceType = serviceType;
    if (!myself.serviceType.endsWith(QChar::fromLatin1('.'))) {
        // mdns.h needs all the qualified names to end with dot for some reason
        myself.serviceType.append(QChar::fromLatin1('.'));
    }
    myself.port = port;
    myself.hostname = QHostInfo::localHostName() + QStringLiteral(".local.");
    myself.serviceInstance = name + QStringLiteral(".") + serviceType;

    /*
    // Maybe pre-compute?
    mdns_string_t service;
    mdns_string_t hostname;
    mdns_string_t service_instance;
    mdns_string_t hostname_qualified;
    mdns_record_t record_ptr;
    mdns_record_t record_srv;
    mdns_record_t record_a;
    mdns_record_t record_aaaa;
    mdns_record_t txt_record[2];

    memset(&service, 0, sizeof(service));
    service.service.str = service_name;
    service.service.length =  strlen(service_name);
    service.hostname.str = hostname;
    service.hostname.length =  strlen(hostname);
    service.service_instance.str = service_instance_buffer;
    service.service_instance.length = strlen(service_instance_buffer);
    service.hostname_qualified.str = qualified_hostname_buffer;
    service.hostname_qualified.length = strlen(qualified_hostname_buffer);
    service.address_ipv4 = service_address_ipv4;
    service.address_ipv6 = service_address_ipv6;
    service.port = service_port;

    // Setup our mDNS records

    // PTR record reverse mapping "<_service-name>._tcp.local." to
    // "<hostname>.<_service-name>._tcp.local."
    service.record_ptr = (mdns_record_t){.name = service.service,
                                         .type = MDNS_RECORDTYPE_PTR,
                                         .data.ptr.name = service.service_instance,
                                         .rclass = 0,
                                         .ttl = 0};

    // SRV record mapping "<hostname>.<_service-name>._tcp.local." to
    // "<hostname>.local." with port. Set weight & priority to 0.
    service.record_srv = (mdns_record_t){.name = service.service_instance,
                                         .type = MDNS_RECORDTYPE_SRV,
                                         .data.srv.name = service.hostname_qualified,
                                         .data.srv.port = (uint16_t)service.port,
                                         .data.srv.priority = 0,
                                         .data.srv.weight = 0,
                                         .rclass = 0,
                                         .ttl = 0};

    // A/AAAA records mapping "<hostname>.local." to IPv4/IPv6 addresses
    service.record_a = (mdns_record_t){.name = service.hostname_qualified,
                                       .type = MDNS_RECORDTYPE_A,
                                       .data.a.addr = service.address_ipv4,
                                       .rclass = 0,
                                       .ttl = 0};

    service.record_aaaa = (mdns_record_t){.name = service.hostname_qualified,
                                          .type = MDNS_RECORDTYPE_AAAA,
                                          .data.aaaa.addr = service.address_ipv6,
                                          .rclass = 0,
                                          .ttl = 0};

    // Add two test TXT records for our service instance name, will be coalesced into
    // one record with both key-value pair strings by the library
    service.txt_record[0] = (mdns_record_t){.name = service.service_instance,
                                            .type = MDNS_RECORDTYPE_TXT,
                                            .data.txt.key = {MDNS_STRING_CONST("test")},
                                            .data.txt.value = {MDNS_STRING_CONST("1")},
                                            .rclass = 0,
                                            .ttl = 0};
    service.txt_record[1] = (mdns_record_t){.name = service.service_instance,
                                            .type = MDNS_RECORDTYPE_TXT,
                                            .data.txt.key = {MDNS_STRING_CONST("other")},
                                            .data.txt.value = {MDNS_STRING_CONST("value")},
                                            .rclass = 0,
                                            .ttl = 0};

    */
}

void MdnsServiceAnnouncer::startAnnouncing()
{
    int num_sockets = listenForQueries();
    if (num_sockets <= 0) {
        qWarning() << "Failed to open any client sockets";
        return;
    }
    sendMulticastAnnounce(false);
}

void MdnsServiceAnnouncer::stopAnnouncing()
{
    sendMulticastAnnounce(true);
    stopListeningForQueries();
}


void MdnsServiceAnnouncer::stopListeningForQueries()
{
    if (socketNotifier != nullptr) {
        delete socketNotifier;
        socketNotifier = nullptr;
    }
    if (socketNotifierV6 != nullptr) {
        delete socketNotifierV6;
        socketNotifierV6 = nullptr;
    }
}

void MdnsServiceAnnouncer::sendMulticastAnnounce(bool isGoodbye)
{
    size_t capacity = 2048;
    void *buffer = malloc(capacity);

    /*
    mdns_record_t additional[5] = {0};
    size_t additional_count = 0;
    additional[additional_count++] = service.record_srv;
    if (service.address_ipv4.sin_family == AF_INET)
        additional[additional_count++] = service.record_a;
    if (service.address_ipv6.sin6_family == AF_INET6)
        additional[additional_count++] = service.record_aaaa;
    additional[additional_count++] = service.txt_record[0];
    additional[additional_count++] = service.txt_record[1];

    if (isGoodbye) {
        qCDebug(KDECONNECT_CORE) << "Sending goodbye";
        if (socketNotifier) mdns_goodbye_multicast(socketNotifier->socket(), buffer, capacity, service.record_ptr, 0, 0, additional, additional_count);
        if (socketNotifierV6) mdns_goodbye_multicast(socketNotifierV6->socket(), buffer, capacity, service.record_ptr, 0, 0, additional, additional_count);
    } else {
        qCDebug(KDECONNECT_CORE) << "Sending announce";
        if (socketNotifier) mdns_announce_multicast(socketNotifier->socket(), buffer, capacity, service.record_ptr, 0, 0, additional, additional_count);
        if (socketNotifierV6) mdns_announce_multicast(socketNotifierV6->socket(), buffer, capacity, service.record_ptr, 0, 0, additional, additional_count);
    }
    */

    free(buffer);
}
