#include <snnetwork/udp.h>

#if defined(SN_OS_LINUX) || defined(SN_OS_MAC)

    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <string.h>
    #include <sys/socket.h>
    #include <sys/time.h>
    #include <unistd.h>

static int get_fd(const SnUdpSocket *sock) {
    int fd;
    memcpy(&fd, sock->data, sizeof(fd));
    return fd;
}

static void set_fd(SnUdpSocket *sock, int fd) {
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

bool sn_udp_open(SnUdpSocket *sock) {
    if (!sock) return false;

    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) return false;

    set_fd(sock, fd);
    return true;
}

bool sn_udp_bind(SnUdpSocket *sock, const SnEndPoint *ep) {
    if (!sock || !ep) return false;

    int fd = get_fd(sock);
    if (fd < 0) return false;

    struct sockaddr_storage ss;
    socklen_t addrlen;
    if (!ep_to_sockaddr(ep, &ss, &addrlen)) return false;

    return bind(fd, (struct sockaddr *)&ss, addrlen) == 0;
}

int64_t sn_udp_send_to(SnUdpSocket *sock, const void *data, uint64_t size, const SnEndPoint *ep) {
    if (!sock || !ep) return -1;

    int fd = get_fd(sock);
    if (fd < 0) return -1;

    struct sockaddr_storage ss;
    socklen_t addrlen;
    if (!ep_to_sockaddr(ep, &ss, &addrlen)) return -1;

    if (size > INT64_MAX) size = INT64_MAX;
    ssize_t ret = sendto(fd, data, (size_t)size, 0, (struct sockaddr *)&ss, addrlen);
    return ret < 0 ? -1 : (int64_t)ret;
}

int64_t sn_udp_receive_from(SnUdpSocket *sock, void *buf, uint64_t size, SnEndPoint *sender_ep) {
    if (!sock) return -1;

    int fd = get_fd(sock);
    if (fd < 0) return -1;

    struct sockaddr_storage ss;
    socklen_t addrlen = sizeof(ss);

    if (size > INT64_MAX) size = INT64_MAX;
    ssize_t ret = recvfrom(fd, buf, (size_t)size, 0, (struct sockaddr *)&ss, &addrlen);
    if (ret < 0) return -1;

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
    int fd = get_fd(sock);
    if (fd >= 0) {
        close(fd);
        set_fd(sock, -1);
    }
}

bool sn_udp_set_timeout(SnUdpSocket *sock, uint64_t timeout_ms) {
    if (!sock) return false;
    int fd = get_fd(sock);
    if (fd < 0) return false;

    struct timeval tv;
    tv.tv_sec = (long)(timeout_ms / 1000);
    tv.tv_usec = (long)((timeout_ms % 1000) * 1000);

    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == 0;
}

#endif
