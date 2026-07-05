#include <snnetwork/network.h>
#include <stdio.h>

int main(void) {
    if (!sn_network_init()) {
        printf("FAIL: sn_network_init\n");
        return 1;
    }

    SnEndPoint ep;
    uint8_t addr[4] = {127, 0, 0, 1};
    sn_end_point_create_ipv4(&ep, addr, 8080);

    char buf[SN_ENDPOINT_STRING_MAX_LENGTH];
    sn_end_point_to_string(&ep, buf);
    printf("Endpoint: %s\n", buf);

    SnEndPoint parsed;
    if (sn_end_point_from_string(&parsed, "192.168.1.1:53")) {
        sn_end_point_to_string(&parsed, buf);
        printf("Parsed: %s\n", buf);
    }

    if (sn_end_point_from_string(&parsed, "[::1]:80")) {
        sn_end_point_to_string(&parsed, buf);
        printf("Parsed: %s\n", buf);
    }

    sn_network_deinit();
    printf("ALL TESTS PASSED\n");
    return 0;
}
