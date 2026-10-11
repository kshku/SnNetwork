#include "../internal.h"

#if defined(SN_OS_LINUX) || defined(SN_OS_MAC)

    #include <fcntl.h>

bool sn_network_set_close_on_exec(intptr_t handle) {
    int fd = (int)handle;
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0) return false;
    return fcntl(fd, F_SETFD, flags | FD_CLOEXEC) == 0;
}

#endif
