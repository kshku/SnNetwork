#include <snnetwork/udp.h>

#if defined(SN_OS_WINDOWS)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>

static SOCKET get_fd(const SnUdpSocket *sock) {
    SOCKET fd;
    memcpy(&fd, sock->data, sizeof(fd));
    return fd;
}

static void set_fd(SnUdpSocket *sock, SOCKET fd) {
    memcpy(sock->data, &fd, sizeof(fd));
}

static bool ep_to_sockaddr(const SnEndPoint *ep, struct sockaddr_storage *ss, int *len) {
    if (!ep) return false;
    uint16_t family;
    memcpy(&family, ep->data, sizeof(family));

    memset(ss, 0, sizeof(*ss));
    if (family == AF_INET) {
        struct sockaddr_in sin;
        memcpy(&sin, ep->data, sizeof(sin));
        memcpy(ss, &sin, sizeof(sin));
        *len = sizeof(sin);
        return true;
    }
    if (family == AF_INET6) {
        struct sockaddr_in6 sin6;
        memcpy(&sin6, ep->data, sizeof(sin6));
        memcpy(ss, &sin6, sizeof(sin6));
        *len = sizeof(sin6);
        return true;
    }
    return false;
}

bool sn_udp_open(SnUdpSocket *sock) {
    if (!sock) return false;

    SOCKET fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == INVALID_SOCKET) return false;

    set_fd(sock, fd);
    return true;
}

bool sn_udp_bind(SnUdpSocket *sock, const SnEndPoint *ep) {
    if (!sock || !ep) return false;

    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return false;

    struct sockaddr_storage ss;
    int addrlen;
    if (!ep_to_sockaddr(ep, &ss, &addrlen))
        return false;

    return bind(fd, (struct sockaddr *)&ss, addrlen) != SOCKET_ERROR;
}

int64_t sn_udp_send_to(SnUdpSocket *sock, const void *data, uint64_t size, const SnEndPoint *ep) {
    if (!sock || !ep) return -1;

    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return -1;

    struct sockaddr_storage ss;
    int addrlen;
    if (!ep_to_sockaddr(ep, &ss, &addrlen))
        return -1;

    if (size > INT_MAX) size = INT_MAX;
    int ret = sendto(fd, (const char *)data, (int)size, 0, (struct sockaddr *)&ss, addrlen);
    return ret == SOCKET_ERROR ? -1 : (int64_t)ret;
}

int64_t sn_udp_receive_from(SnUdpSocket *sock, void *buf, uint64_t size, SnEndPoint *sender_ep) {
    if (!sock) return -1;

    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return -1;

    struct sockaddr_storage ss;
    int addrlen = sizeof(ss);

    if (size > INT_MAX) size = INT_MAX;
    int ret = recvfrom(fd, (char *)buf, (int)size, 0, (struct sockaddr *)&ss, &addrlen);
    if (ret == SOCKET_ERROR) return -1;

    if (sender_ep) {
        memset(sender_ep->data, 0, sizeof(sender_ep->data));
        if (ss.ss_family == AF_INET) {
            struct sockaddr_in sin;
            memcpy(&sin, &ss, sizeof(sin));
            memcpy(sender_ep->data, &sin, sizeof(sin));
        } else if (ss.ss_family == AF_INET6) {
            struct sockaddr_in6 sin6;
            memcpy(&sin6, &ss, sizeof(sin6));
            memcpy(sender_ep->data, &sin6, sizeof(sin6));
        }
    }

    return (int64_t)ret;
}

void sn_udp_close(SnUdpSocket *sock) {
    if (!sock) return;
    SOCKET fd = get_fd(sock);
    if (fd != INVALID_SOCKET) {
        closesocket(fd);
        set_fd(sock, INVALID_SOCKET);
    }
}

bool sn_udp_set_timeout(SnUdpSocket *sock, uint64_t timeout_ms) {
    if (!sock) return false;
    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return false;

    DWORD tv = (DWORD)timeout_ms;
    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) == 0;
}

#endif
