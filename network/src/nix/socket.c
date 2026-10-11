#include <snnetwork/socket.h>

#if defined(SN_OS_LINUX) || defined(SN_OS_MAC)

    #include <arpa/inet.h>
    #include <errno.h>
    #include <limits.h>
    #include <netinet/in.h>
    #include <poll.h>
    #include <sntime/sntime.h>
    #include <stdlib.h>
    #include <string.h>
    #include <sys/socket.h>

SN_STATIC_ASSERT(sizeof(int) <= sizeof(SnSocket), "SnSocket must hold a POSIX file descriptor");

static int get_fd(const SnSocket *sock) {
    int fd;
    memcpy(&fd, sock->data, sizeof(fd));
    return fd;
}

/* A peer close (FIN) is reported by poll() as readability, not as POLLHUP, so a
   readable socket is probed with MSG_PEEK: a zero-length result means the peer
   sent FIN. A listening socket or a socket with data pending returns > 0 or -1
   and is not treated as hung up. */
static bool is_hung_up(int fd) {
    char probe;
    return recv(fd, &probe, 1, MSG_PEEK) == 0;
}

SnWaitStatus sn_socket_wait(SnPollFd *fds, size_t count, uint64_t timeout_ms) {
    if (!fds && count > 0) return SN_WAIT_ERROR;

    struct pollfd stack_pfds[64];
    struct pollfd *pfds = stack_pfds;
    bool heap = false;
    if (count > SN_ARRAY_LENGTH(stack_pfds)) {
        pfds = malloc(count * sizeof(*pfds));
        if (!pfds) return SN_WAIT_ERROR;
        heap = true;
    }

    for (size_t i = 0; i < count; ++i) {
        fds[i].readable = false;
        fds[i].hangup = false;
        pfds[i].fd = fds[i].sock ? get_fd(fds[i].sock) : -1;
        pfds[i].events = POLLIN;
        pfds[i].revents = 0;
    }

    bool infinite = timeout_ms == SN_TIMEOUT_INFINITE;
    uint64_t deadline = 0;
    if (!infinite) {
        uint64_t delta = timeout_ms > (uint64_t)INT64_MAX ? (uint64_t)INT64_MAX : timeout_ms;
        deadline = (uint64_t)sn_time_now_ms() + delta;
    }

    int ret;
    for (;;) {
        int wait;
        if (infinite) {
            wait = -1;
        } else {
            uint64_t now = (uint64_t)sn_time_now_ms();
            if (now >= deadline) {
                wait = 0;
            } else {
                uint64_t remaining = deadline - now;
                wait = remaining > (uint64_t)INT_MAX ? INT_MAX : (int)remaining;
            }
        }

        ret = poll(pfds, (nfds_t)count, wait);
        if (ret >= 0) break;
        if (errno != EINTR) {
            if (heap) free(pfds);
            return SN_WAIT_ERROR;
        }
    }

    if (ret == 0) {
        if (heap) free(pfds);
        return SN_WAIT_TIMEOUT;
    }

    for (size_t i = 0; i < count; ++i) {
        short revents = pfds[i].revents;
        fds[i].readable = revents != 0;
        fds[i].hangup = (revents & (POLLHUP | POLLERR | POLLNVAL)) != 0;

        if (fds[i].readable && !fds[i].hangup && pfds[i].fd >= 0) {
            fds[i].hangup = is_hung_up(pfds[i].fd);
        }
    }

    if (heap) free(pfds);
    return SN_WAIT_READY;
}

bool sn_socket_get_local_end_point(const SnSocket *sock, SnEndPoint *ep) {
    if (!sock || !ep) return false;

    int fd = get_fd(sock);
    if (fd < 0) return false;

    struct sockaddr_storage ss;
    socklen_t len = sizeof(ss);
    if (getsockname(fd, (struct sockaddr *)&ss, &len) < 0) return false;

    memset(ep->data, 0, sizeof(ep->data));
    if (ss.ss_family == AF_INET) {
        struct sockaddr_in sin;
        memcpy(&sin, &ss, sizeof(sin));
        memcpy(ep->data, &sin, sizeof(sin));
    } else if (ss.ss_family == AF_INET6) {
        struct sockaddr_in6 sin6;
        memcpy(&sin6, &ss, sizeof(sin6));
        memcpy(ep->data, &sin6, sizeof(sin6));
    } else {
        return false;
    }

    return true;
}

#endif
