#include <snnetwork/tcp.h>

#if defined(SN_OS_LINUX) || defined(SN_OS_MAC)

#include <arpa/inet.h>
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

    memset(ss, 0, sizeof(*ss));
    memcpy(ss, ep->data, sizeof(*ss));
    if (ss->ss_family == AF_INET) {
        *len = sizeof(struct sockaddr_in);
        return true;
    }
    if (ss->ss_family == AF_INET6) {
        *len = sizeof(struct sockaddr_in6);
        return true;
    }
    return false;
}

bool sn_tcp_listen(SnTcpSocket *sock, const SnEndPoint *ep, int backlog) {
    if (!sock) return false;

    struct sockaddr_storage ss;
    socklen_t addrlen;
    if (ep && !ep_to_sockaddr(ep, &ss, &addrlen))
        return false;

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

bool sn_tcp_accept(SnTcpSocket *sock, SnTcpSocket *client, SnEndPoint *client_ep) {
    if (!sock || !client) return false;

    int fd = get_fd(sock);
    if (fd < 0) return false;

    struct sockaddr_storage ss;
    socklen_t addrlen = sizeof(ss);
    int client_fd = accept(fd, (struct sockaddr *)&ss, &addrlen);
    if (client_fd < 0) return false;

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
    socklen_t addrlen;
    if (!ep_to_sockaddr(ep, &ss, &addrlen))
        return false;

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

int64_t sn_tcp_receive(SnTcpSocket *sock, void *buf, uint64_t size) {
    if (!sock) return -1;

    int fd = get_fd(sock);
    if (fd < 0) return -1;

    if (size > INT64_MAX) size = INT64_MAX;
    ssize_t ret = recv(fd, buf, (size_t)size, 0);
    return ret < 0 ? -1 : (int64_t)ret;
}

void sn_tcp_close(SnTcpSocket *sock) {
    if (!sock) return;
    int fd = get_fd(sock);
    if (fd >= 0) {
        close(fd);
        set_fd(sock, -1);
    }
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

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
        return false;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv)) < 0)
        return false;

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
