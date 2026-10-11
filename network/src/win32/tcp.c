#include "../internal.h"

#include <snnetwork/tcp.h>

#if defined(SN_OS_WINDOWS)

    #include <string.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>

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
    if (ep && !ep_to_sockaddr(ep, &ss, &addrlen)) return false;

    int domain = ep ? ss.ss_family : AF_INET;
    SOCKET fd = socket(domain, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) return false;
    if (!sn_network_set_close_on_exec((intptr_t)fd)) {
        closesocket(fd);
        return false;
    }

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

SnAcceptStatus sn_tcp_accept(SnTcpSocket *sock, SnTcpSocket *client, SnEndPoint *client_ep, uint64_t timeout_ms) {
    if (!sock || !client) return SN_ACCEPT_ERROR;

    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return SN_ACCEPT_ERROR;

    SnPollFd pfd;
    pfd.sock = sock;
    pfd.readable = false;
    pfd.hangup = false;
    SnWaitStatus wait = sn_socket_wait(&pfd, 1, timeout_ms);
    if (wait == SN_WAIT_ERROR) return SN_ACCEPT_ERROR;
    if (wait == SN_WAIT_TIMEOUT) return SN_ACCEPT_TIMEOUT;

    struct sockaddr_storage ss;
    int addrlen = sizeof(ss);
    SOCKET client_fd = accept(fd, (struct sockaddr *)&ss, &addrlen);
    if (client_fd == INVALID_SOCKET) {
        int error = WSAGetLastError();
        if (error == WSAEWOULDBLOCK || error == WSAEINTR) return SN_ACCEPT_TIMEOUT;
        return SN_ACCEPT_ERROR;
    }
    if (!sn_network_set_close_on_exec((intptr_t)client_fd)) {
        closesocket(client_fd);
        return SN_ACCEPT_ERROR;
    }

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

    return SN_ACCEPT_OK;
}

bool sn_tcp_connect(SnTcpSocket *sock, const SnEndPoint *ep) {
    if (!sock || !ep) return false;

    struct sockaddr_storage ss;
    int addrlen;
    if (!ep_to_sockaddr(ep, &ss, &addrlen)) return false;

    SOCKET fd = socket(ss.ss_family, SOCK_STREAM, 0);
    if (fd == INVALID_SOCKET) return false;
    if (!sn_network_set_close_on_exec((intptr_t)fd)) {
        closesocket(fd);
        return false;
    }

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

int64_t sn_tcp_receive(SnTcpSocket *sock, void *buf, uint64_t size, SnRecvStatus *status) {
    if (status) *status = SN_RECV_ERROR;
    if (!sock || !buf) return -1;

    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return -1;

    if (size > INT_MAX) size = INT_MAX;
    int ret = recv(fd, (char *)buf, (int)size, 0);
    if (ret == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (status)
            *status = (error == WSAETIMEDOUT || error == WSAEWOULDBLOCK) ? SN_RECV_TIMEOUT : SN_RECV_ERROR;
        return -1;
    }
    if (ret == 0) {
        if (status) *status = SN_RECV_CLOSED;
        return 0;
    }

    if (status) *status = SN_RECV_DATA;
    return (int64_t)ret;
}

void sn_tcp_close(SnTcpSocket *sock) {
    if (!sock) return;
    SOCKET fd = get_fd(sock);
    if (fd != INVALID_SOCKET) {
        closesocket(fd);
        set_fd(sock, INVALID_SOCKET);
    }
}

bool sn_tcp_shutdown(SnTcpSocket *sock, SnShutdownHow how) {
    if (!sock) return false;

    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return false;

    int direction;
    switch (how) {
        case SN_SHUTDOWN_RD:
            direction = SD_RECEIVE;
            break;
        case SN_SHUTDOWN_WR:
            direction = SD_SEND;
            break;
        case SN_SHUTDOWN_BOTH:
            direction = SD_BOTH;
            break;
        default:
            return false;
    }

    return shutdown(fd, direction) != SOCKET_ERROR;
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
