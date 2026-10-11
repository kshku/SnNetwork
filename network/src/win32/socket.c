#include <snnetwork/socket.h>

#if defined(SN_OS_WINDOWS)

    #include "../internal.h"

    #include <limits.h>
    #include <string.h>
    #include <winsock2.h>
    #include <ws2tcpip.h>

SN_STATIC_ASSERT(sizeof(SOCKET) <= sizeof(SnSocket), "SnSocket must hold a Win32 socket");

static SOCKET get_fd(const SnSocket *sock) {
    SOCKET fd;
    memcpy(&fd, sock->data, sizeof(fd));
    return fd;
}

/* WSAPoll reports a peer close as readability, not always as POLLHUP. Probe a
   readable socket with MSG_PEEK: a zero-length result means the peer sent FIN. */
static bool is_hung_up(SOCKET fd) {
    char probe;
    return recv(fd, &probe, 1, MSG_PEEK) == 0;
}

SnWaitStatus sn_socket_wait(SnPollFd *fds, size_t count, uint64_t timeout_ms) {
    if (!fds && count > 0) return SN_WAIT_ERROR;

    WSAPOLLFD stack_pfds[64];
    WSAPOLLFD *pfds = stack_pfds;
    /* Sets that fit the stack scratch never allocate; larger sets use the
       caller-installed allocator and fail when none is provided. */
    const SnMemoryAllocator *scratch = NULL;
    if (count > SN_ARRAY_LENGTH(stack_pfds)) {
        scratch = sn_network_get_allocator();
        if (!scratch || !scratch->alloc || !scratch->free) return SN_WAIT_ERROR;
        pfds = scratch->alloc(scratch->data, (uint64_t)(count * sizeof(*pfds)), alignof(WSAPOLLFD));
        if (!pfds) return SN_WAIT_ERROR;
    }

    for (size_t i = 0; i < count; ++i) {
        fds[i].readable = false;
        fds[i].hangup = false;
        pfds[i].fd = fds[i].sock ? get_fd(fds[i].sock) : INVALID_SOCKET;
        pfds[i].events = POLLRDNORM;
        pfds[i].revents = 0;
    }

    int wait;
    if (timeout_ms == SN_TIMEOUT_INFINITE) {
        wait = -1;
    } else {
        wait = timeout_ms > (uint64_t)INT_MAX ? INT_MAX : (int)timeout_ms;
    }

    int ret = WSAPoll(pfds, (ULONG)count, wait);
    if (ret == SOCKET_ERROR) {
        if (scratch) scratch->free(scratch->data, pfds);
        return SN_WAIT_ERROR;
    }

    if (ret == 0) {
        if (scratch) scratch->free(scratch->data, pfds);
        return SN_WAIT_TIMEOUT;
    }

    for (size_t i = 0; i < count; ++i) {
        short revents = pfds[i].revents;
        fds[i].readable = revents != 0;
        fds[i].hangup = (revents & (POLLHUP | POLLERR | POLLNVAL)) != 0;

        if (fds[i].readable && !fds[i].hangup && pfds[i].fd != INVALID_SOCKET) {
            fds[i].hangup = is_hung_up(pfds[i].fd);
        }
    }

    if (scratch) scratch->free(scratch->data, pfds);
    return SN_WAIT_READY;
}

bool sn_socket_get_local_end_point(const SnSocket *sock, SnEndPoint *ep) {
    if (!sock || !ep) return false;

    SOCKET fd = get_fd(sock);
    if (fd == INVALID_SOCKET) return false;

    struct sockaddr_storage ss;
    int len = sizeof(ss);
    if (getsockname(fd, (struct sockaddr *)&ss, &len) == SOCKET_ERROR) return false;

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
