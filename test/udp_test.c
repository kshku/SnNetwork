#include <snnetwork/network.h>
#include <stdio.h>
#include <string.h>

#define TEST_PORT 18766

int main(void) {
    if (!sn_network_init()) {
        printf("FAIL: sn_network_init\n");
        return 1;
    }

    SnEndPoint server_ep;
    uint8_t addr[4] = {127, 0, 0, 1};
    sn_end_point_create_ipv4(&server_ep, addr, TEST_PORT);

    SnUdpSocket sock_a;
    if (!sn_udp_open(&sock_a)) {
        printf("FAIL: sn_udp_open A\n");
        sn_network_deinit();
        return 1;
    }
    if (!sn_udp_bind(&sock_a, &server_ep)) {
        printf("FAIL: sn_udp_bind A\n");
        sn_udp_close(&sock_a);
        sn_network_deinit();
        return 1;
    }

    SnUdpSocket sock_b;
    if (!sn_udp_open(&sock_b)) {
        printf("FAIL: sn_udp_open B\n");
        sn_udp_close(&sock_a);
        sn_network_deinit();
        return 1;
    }

    const char *msg = "hello";
    int64_t sent = sn_udp_send_to(&sock_b, msg, strlen(msg), &server_ep);
    if (sent != (int64_t)strlen(msg)) {
        printf("FAIL: sn_udp_send_to\n");
        sn_udp_close(&sock_b);
        sn_udp_close(&sock_a);
        sn_network_deinit();
        return 1;
    }

    char recv_buf[64] = {0};
    SnEndPoint sender_ep;
    int64_t received = sn_udp_receive_from(&sock_a, recv_buf, sizeof(recv_buf), &sender_ep);
    if (received != (int64_t)strlen(msg) || strcmp(recv_buf, msg) != 0) {
        printf("FAIL: sn_udp_receive_from (got: %s)\n", recv_buf);
        sn_udp_close(&sock_b);
        sn_udp_close(&sock_a);
        sn_network_deinit();
        return 1;
    }

    const char *response = "world";
    sent = sn_udp_send_to(&sock_a, response, strlen(response), &sender_ep);
    if (sent != (int64_t)strlen(response)) {
        printf("FAIL: sn_udp_send_to response\n");
        sn_udp_close(&sock_b);
        sn_udp_close(&sock_a);
        sn_network_deinit();
        return 1;
    }

    memset(recv_buf, 0, sizeof(recv_buf));
    SnEndPoint responder_ep;
    received = sn_udp_receive_from(&sock_b, recv_buf, sizeof(recv_buf), &responder_ep);
    if (received != (int64_t)strlen(response) || strcmp(recv_buf, response) != 0) {
        printf("FAIL: sn_udp_receive_from response (got: %s)\n", recv_buf);
        sn_udp_close(&sock_b);
        sn_udp_close(&sock_a);
        sn_network_deinit();
        return 1;
    }

    sn_udp_close(&sock_b);
    sn_udp_close(&sock_a);
    sn_network_deinit();

    printf("UDP TEST PASSED\n");
    return 0;
}
