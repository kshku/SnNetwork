#include <snnetwork/tcp.h>

#if defined(SN_OS_LINUX) || defined(SN_OS_MAC)

    #include <arpa/inet.h>
    #include <errno.h>
    #include <netinet/in.h>
    #include <netinet/tcp.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <unistd.h>

static int get_fd(const SnTcpSocket *sock) {
    int fd;
    memcpy(&fd, sock->data, sizeof(fd));
    return fd;
}

static void set_fd(SnTcpSocket *sock, int fd) {
    memcpy(sock->data, &fd, sizeof(fd));
}

static bool ep_to_sockaddr(const SnEndPoint *ep, struct sockaddr_storage *ss, socklen_t *len) {
    if (!ep) return false;

    struct sockaddr sa;
    memcpy(&sa, ep->data, sizeof(sa));

    memset(ss, 0, sizeof(*ss));
    if (sa.sa_family == AF_INET) {
        memcpy(ss, ep->data, sizeof(struct sockaddr_in));
        *len = sizeof(struct sockaddr_in);
        return true;
    }
    if (sa.sa_family == AF_INET6) {
        memcpy(ss, ep->data, sizeof(struct sockaddr_in6));
        *len = sizeof(struct sockaddr_in6);
        return true;
    }
    return false;
}

bool sn_tcp_listen(SnTcpSocket *sock, const SnEndPoint *ep, int backlog) {
    if (!sock) return false;

    struct sockaddr_storage ss;
    socklen_t addrlen;
    if (ep && !ep_to_sockaddr(ep, &ss, &addrlen)) return false;

    int domain = ep ? ss.ss_family : AF_INET;
    int fd = socket(domain, SOCK_STREAM, 0);
    if (fd < 0) return false;

    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    if (ep) {
        if (bind(fd, (struct sockaddr *)&ss, addrlen) < 0) {
            close(fd);
            return false;
        }
    }

    if (backlog < 0) backlog = SOMAXCONN;
    if (listen(fd, backlog) < 0) {
        close(fd);
        return false;
    }

    set_fd(sock, fd);
    return true;
}

SnAcceptStatus sn_tcp_accept(SnTcpSocket *sock, SnTcpSocket *client, SnEndPoint *client_ep, uint64_t timeout_ms) {
    if (!sock || !client) return SN_ACCEPT_ERROR;

    int fd = get_fd(sock);
    if (fd < 0) return SN_ACCEPT_ERROR;

    SnPollFd pfd;
    pfd.sock = sock;
    pfd.readable = false;
    pfd.hangup = false;
    SnWaitStatus wait = sn_socket_wait(&pfd, 1, timeout_ms);
    if (wait == SN_WAIT_ERROR) return SN_ACCEPT_ERROR;
    if (wait == SN_WAIT_TIMEOUT) return SN_ACCEPT_TIMEOUT;

    struct sockaddr_storage ss;
    socklen_t addrlen = sizeof(ss);
    int client_fd = accept(fd, (struct sockaddr *)&ss, &addrlen);
    if (client_fd < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return SN_ACCEPT_TIMEOUT;
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
    socklen_t addrlen;
    if (!ep_to_sockaddr(ep, &ss, &addrlen)) return false;

    int fd = socket(ss.ss_family, SOCK_STREAM, 0);
    if (fd < 0) return false;

    if (connect(fd, (struct sockaddr *)&ss, addrlen) < 0) {
        close(fd);
        return false;
    }

    set_fd(sock, fd);
    return true;
}

int64_t sn_tcp_send(SnTcpSocket *sock, const void *data, uint64_t size) {
    if (!sock) return -1;

    int fd = get_fd(sock);
    if (fd < 0) return -1;

    if (size > INT64_MAX) size = INT64_MAX;
    ssize_t ret = send(fd, data, (size_t)size, 0);
    return ret < 0 ? -1 : (int64_t)ret;
}

int64_t sn_tcp_receive(SnTcpSocket *sock, void *buf, uint64_t size, SnRecvStatus *status) {
    if (status) *status = SN_RECV_ERROR;
    if (!sock || !buf) return -1;

    int fd = get_fd(sock);
    if (fd < 0) return -1;

    if (size > INT64_MAX) size = INT64_MAX;

    ssize_t ret;
    do {
        ret = recv(fd, buf, (size_t)size, 0);
    } while (ret < 0 && errno == EINTR);

    if (ret < 0) {
        if (status)
            *status = (errno == EAGAIN || errno == EWOULDBLOCK) ? SN_RECV_TIMEOUT : SN_RECV_ERROR;
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
    int fd = get_fd(sock);
    if (fd >= 0) {
        close(fd);
        set_fd(sock, -1);
    }
}

bool sn_tcp_shutdown(SnTcpSocket *sock, SnShutdownHow how) {
    if (!sock) return false;

    int fd = get_fd(sock);
    if (fd < 0) return false;

    int direction;
    switch (how) {
        case SN_SHUTDOWN_RD:
            direction = SHUT_RD;
            break;
        case SN_SHUTDOWN_WR:
            direction = SHUT_WR;
            break;
        case SN_SHUTDOWN_BOTH:
            direction = SHUT_RDWR;
            break;
        default:
            return false;
    }

    return shutdown(fd, direction) == 0;
}

bool sn_tcp_set_nodelay(SnTcpSocket *sock, bool enable) {
    if (!sock) return false;
    int fd = get_fd(sock);
    if (fd < 0) return false;
    int optval = enable ? 1 : 0;
    return setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval)) == 0;
}

bool sn_tcp_set_timeout(SnTcpSocket *sock, uint64_t timeout_ms) {
    if (!sock) return false;
    int fd = get_fd(sock);
    if (fd < 0) return false;

    struct timeval tv;
    tv.tv_sec = (long)(timeout_ms / 1000);
    tv.tv_usec = (long)((timeout_ms % 1000) * 1000);

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) return false;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0) return false;

    return true;
}

bool sn_tcp_set_keepalive(SnTcpSocket *sock, bool enable) {
    if (!sock) return false;
    int fd = get_fd(sock);
    if (fd < 0) return false;
    int optval = enable ? 1 : 0;
    return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval)) == 0;
}

#endif
