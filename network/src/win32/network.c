#include <snnetwork/network.h>

#if defined(SN_OS_WINDOWS)

    #include <winsock2.h>

bool sn_network_init(void) {
    WSADATA wsa;
    return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
}

void sn_network_deinit(void) {
    WSACleanup();
}

#endif
