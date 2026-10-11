#include "../internal.h"

#if defined(SN_OS_WINDOWS)

    #include <windows.h>

bool sn_network_set_close_on_exec(intptr_t handle) {
    return SetHandleInformation((HANDLE)handle, HANDLE_FLAG_INHERIT, 0) != 0;
}

#endif
