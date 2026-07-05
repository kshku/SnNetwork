#include <snnetwork/network.h>

#if defined(SN_OS_LINUX) || defined(SN_OS_MAC)

bool sn_network_init(void) {
    return true;
}

void sn_network_deinit(void) {
}

#endif
