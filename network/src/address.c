#include <snnetwork/address.h>

#if defined(SN_OS_LINUX) || defined(SN_OS_MAC)
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <sys/socket.h>
#elif defined(SN_OS_WINDOWS)
    #include <winsock2.h>
    #include <ws2tcpip.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

SN_STATIC_ASSERT(sizeof(struct sockaddr_in6) <= sizeof(SnEndPoint), "SnEndPoint must be large "
                                                                    "enough for sockaddr_in6");

void sn_end_point_create_ipv4(SnEndPoint *ep, const uint8_t addr[4], uint16_t port) {
    struct sockaddr_in sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    memcpy(&sin.sin_addr, addr, 4);
    memcpy(ep->data, &sin, sizeof(sin));
    memset(ep->data + sizeof(sin), 0, sizeof(ep->data) - sizeof(sin));
}

void sn_end_point_create_ipv6(SnEndPoint *ep, const uint8_t addr[16], uint16_t port, uint32_t scope_id) {
    struct sockaddr_in6 sin6;
    memset(&sin6, 0, sizeof(sin6));
    sin6.sin6_family = AF_INET6;
    sin6.sin6_port = htons(port);
    sin6.sin6_scope_id = scope_id;
    memcpy(&sin6.sin6_addr, addr, 16);
    memcpy(ep->data, &sin6, sizeof(sin6));
    memset(ep->data + sizeof(sin6), 0, sizeof(ep->data) - sizeof(sin6));
}

bool sn_end_point_from_string(SnEndPoint *ep, const char *str) {
    if (!ep || !str) return false;

    if (str[0] == '[') {
        const char *close_bracket = strchr(str, ']');
        if (!close_bracket) return false;

        size_t addr_len = (size_t)(close_bracket - str - 1);
        if (addr_len >= 64) return false;

        char addr_str[64];
        memcpy(addr_str, str + 1, addr_len);
        addr_str[addr_len] = '\0';

        uint32_t scope_id = 0;
        char *pct = strchr(addr_str, '%');
        if (pct) {
            *pct = '\0';
            unsigned long idx = strtoul(pct + 1, NULL, 10);
            if (idx > 0 && idx <= UINT32_MAX) scope_id = (uint32_t)idx;
        }

        if (close_bracket[1] != ':') return false;
        long port = strtol(close_bracket + 2, NULL, 10);
        if (port <= 0 || port > 65535) return false;

        struct in6_addr v6;
        if (inet_pton(AF_INET6, addr_str, &v6) != 1) return false;

        sn_end_point_create_ipv6(ep, (const uint8_t *)&v6, (uint16_t)port, scope_id);
        return true;
    }

    const char *colon = strrchr(str, ':');
    if (!colon) return false;

    size_t addr_len = (size_t)(colon - str);
    if (addr_len >= 64) return false;

    char addr_str[64];
    memcpy(addr_str, str, addr_len);
    addr_str[addr_len] = '\0';

    long port = strtol(colon + 1, NULL, 10);
    if (port <= 0 || port > 65535) return false;

    struct in_addr v4;
    if (inet_pton(AF_INET, addr_str, &v4) != 1) return false;

    sn_end_point_create_ipv4(ep, (const uint8_t *)&v4, (uint16_t)port);
    return true;
}

uint16_t sn_end_point_get_port(const SnEndPoint *ep) {
    if (!ep) return 0;
    uint16_t port;
    memcpy(&port, ep->data + 2, sizeof(port));
    return ntohs(port);
}

SnAddressFamily sn_end_point_get_family(const SnEndPoint *ep) {
    if (!ep) return SN_ADDRESS_FAMILY_UNSPEC;
    struct sockaddr sa;
    memcpy(&sa, ep->data, sizeof(sa));
    if (sa.sa_family == AF_INET) return SN_ADDRESS_FAMILY_IPV4;
    if (sa.sa_family == AF_INET6) return SN_ADDRESS_FAMILY_IPV6;
    return SN_ADDRESS_FAMILY_UNSPEC;
}

void sn_end_point_to_string(const SnEndPoint *ep, char buf[SN_ENDPOINT_STRING_MAX_LENGTH]) {
    if (!ep) {
        buf[0] = '\0';
        return;
    }

    struct sockaddr sa;
    memcpy(&sa, ep->data, sizeof(sa));

    if (sa.sa_family == AF_INET) {
        struct sockaddr_in sin;
        memcpy(&sin, ep->data, sizeof(sin));
        inet_ntop(AF_INET, &sin.sin_addr, buf, SN_ENDPOINT_STRING_MAX_LENGTH);
        size_t len = strlen(buf);
        snprintf(buf + len, SN_ENDPOINT_STRING_MAX_LENGTH - len, ":%u", ntohs(sin.sin_port));
    } else if (sa.sa_family == AF_INET6) {
        struct sockaddr_in6 sin6;
        memcpy(&sin6, ep->data, sizeof(sin6));
        buf[0] = '[';
        inet_ntop(AF_INET6, &sin6.sin6_addr, buf + 1, SN_ENDPOINT_STRING_MAX_LENGTH - 8);
        size_t len = strlen(buf);
        snprintf(buf + len, SN_ENDPOINT_STRING_MAX_LENGTH - len, "]:%u", ntohs(sin6.sin6_port));
    } else {
        buf[0] = '\0';
    }
}

bool sn_end_point_equal(const SnEndPoint *a, const SnEndPoint *b) {
    if (!a || !b) return false;

    struct sockaddr a_sa, b_sa;
    memcpy(&a_sa, a->data, sizeof(a_sa));
    memcpy(&b_sa, b->data, sizeof(b_sa));
    if (a_sa.sa_family != b_sa.sa_family) return false;

    if (a_sa.sa_family == AF_INET) return memcmp(a->data, b->data, sizeof(struct sockaddr_in)) == 0;
    if (a_sa.sa_family == AF_INET6)
        return memcmp(a->data, b->data, sizeof(struct sockaddr_in6)) == 0;
    return false;
}
