#include <snnetwork/tcp.h>

#if defined(SN_OS_WINDOWS)

#include <winsock2.h>
#include <ws2tcpip.h>
#include <string.h>

static SOCKET get_fd(const SnTcpSocket *sock) {
    SOCKET fd;
    memcpy(&fd, sock->data, sizeof(fd));
    return fd;
}

static void set_fd(SnTcpSocket *sock, SOCKET fd) {
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

bool sn_tcp_listen(SnTcpSocket *sock, const SnEndPoint *ep, int backlog) {
    if (!sock) return false;

    struct sockaddr_storage ss;
    int addrlen;
    if (ep && !ep_to_sockaddr(ep, &ss, &addrlen))
        return false;

    int domain = ep ? ss.ss_family : AF_INET;
    SOCKET fd = socket(domain, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) return false;

    char optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (ep) {
        if (bind(fd, (struct sockaddr *)&ss, addrlen) == SOCKET_ERROR) {
            closesocket(fd);
            return false;
        }
    }

    if (backlog < 0) backlog = SOMAXCONN;
    if (listen(fd, backlog) == SOCKET_ERROR) {
        closesocket(fd);
        return false;
    }

    set_fd(sock, fd);
    return true;
}

bool sn_tcp_accept(SnTcpSocket *sock, SnTcpSocket *client, SnEndPoint *client_ep) {
    if (!sock || !client) return false;

    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return false;

    struct sockaddr_storage ss;
    int addrlen = sizeof(ss);
    SOCKET client_fd = accept(fd, (struct sockaddr *)&ss, &addrlen);
    if (client_fd == INVALID_SOCKET) return false;

    set_fd(client, client_fd);

    if (client_ep) {
        memset(client_ep->data, 0, sizeof(client_ep->data));
        if (ss.ss_family == AF_INET) {
            struct sockaddr_in sin;
            memcpy(&sin, &ss, sizeof(sin));
            memcpy(client_ep->data, &sin, sizeof(sin));
        } else if (ss.ss_family == AF_INET6) {
            struct sockaddr_in6 sin6;
            memcpy(&sin6, &ss, sizeof(sin6));
            memcpy(client_ep->data, &sin6, sizeof(sin6));
        }
    }

    return true;
}

bool sn_tcp_connect(SnTcpSocket *sock, const SnEndPoint *ep) {
    if (!sock || !ep) return false;

    struct sockaddr_storage ss;
    int addrlen;
    if (!ep_to_sockaddr(ep, &ss, &addrlen))
        return false;

    SOCKET fd = socket(ss.ss_family, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) return false;

    if (connect(fd, (struct sockaddr *)&ss, addrlen) == SOCKET_ERROR) {
        closesocket(fd);
        return false;
    }

    set_fd(sock, fd);
    return true;
}

int64_t sn_tcp_send(SnTcpSocket *sock, const void *data, uint64_t size) {
    if (!sock) return -1;

    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return -1;

    if (size > INT_MAX) size = INT_MAX;
    int ret = send(fd, (const char *)data, (int)size, 0);
    return ret == SOCKET_ERROR ? -1 : (int64_t)ret;
}

int64_t sn_tcp_receive(SnTcpSocket *sock, void *buf, uint64_t size) {
    if (!sock) return -1;

    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return -1;

    if (size > INT_MAX) size = INT_MAX;
    int ret = recv(fd, (char *)buf, (int)size, 0);
    return ret == SOCKET_ERROR ? -1 : (int64_t)ret;
}

void sn_tcp_close(SnTcpSocket *sock) {
    if (!sock) return;
    SOCKET fd = get_fd(sock);
    if (fd != INVALID_SOCKET) {
        closesocket(fd);
        set_fd(sock, INVALID_SOCKET);
    }
}

bool sn_tcp_set_nodelay(SnTcpSocket *sock, bool enable) {
    if (!sock) return false;
    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return false;
    char optval = enable ? 1 : 0;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) == 0;
}

bool sn_tcp_set_timeout(SnTcpSocket *sock, uint64_t timeout_ms) {
    if (!sock) return false;
    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return false;

    DWORD tv = (DWORD)timeout_ms;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&tv, sizeof(tv)) == SOCKET_ERROR)
        return false;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, (const char *)&tv, sizeof(tv)) == SOCKET_ERROR)
        return false;

    return true;
}

bool sn_tcp_set_keepalive(SnTcpSocket *sock, bool enable) {
    if (!sock) return false;
    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return false;
    char optval = enable ? 1 : 0;
    return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == 0;
}

#endif
