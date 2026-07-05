#include <snnetwork/network.h>
#include <stdio.h>
#include <string.h>

#define TEST_PORT 18765

int main(void) {
    if (!sn_network_init()) {
        printf("FAIL: sn_network_init\n");
        return 1;
    }

    SnEndPoint server_ep;
    uint8_t addr[4] = {127, 0, 0, 1};
    sn_end_point_create_ipv4(&server_ep, addr, TEST_PORT);

    SnTcpSocket listener;
    if (!sn_tcp_listen(&listener, &server_ep, 1)) {
        printf("FAIL: sn_tcp_listen\n");
        sn_network_deinit();
        return 1;
    }

    SnTcpSocket client;
    if (!sn_tcp_connect(&client, &server_ep)) {
        printf("FAIL: sn_tcp_connect\n");
        sn_tcp_close(&listener);
        sn_network_deinit();
        return 1;
    }

    SnTcpSocket server_client;
    if (!sn_tcp_accept(&listener, &server_client, NULL)) {
        printf("FAIL: sn_tcp_accept\n");
        sn_tcp_close(&client);
        sn_tcp_close(&listener);
        sn_network_deinit();
        return 1;
    }

    const char *msg = "hello";
    int64_t sent = sn_tcp_send(&client, msg, strlen(msg));
    if (sent != (int64_t)strlen(msg)) {
        printf("FAIL: sn_tcp_send\n");
        sn_tcp_close(&server_client);
        sn_tcp_close(&client);
        sn_tcp_close(&listener);
        sn_network_deinit();
        return 1;
    }

    char recv_buf[64] = {0};
    int64_t received = sn_tcp_receive(&server_client, recv_buf, sizeof(recv_buf));
    if (received != (int64_t)strlen(msg) || strcmp(recv_buf, msg) != 0) {
        printf("FAIL: sn_tcp_receive (got: %s)\n", recv_buf);
        sn_tcp_close(&server_client);
        sn_tcp_close(&client);
        sn_tcp_close(&listener);
        sn_network_deinit();
        return 1;
    }

    const char *response = "world";
    sent = sn_tcp_send(&server_client, response, strlen(response));
    if (sent != (int64_t)strlen(response)) {
        printf("FAIL: sn_tcp_send response\n");
        sn_tcp_close(&server_client);
        sn_tcp_close(&client);
        sn_tcp_close(&listener);
        sn_network_deinit();
        return 1;
    }

    memset(recv_buf, 0, sizeof(recv_buf));
    received = sn_tcp_receive(&client, recv_buf, sizeof(recv_buf));
    if (received != (int64_t)strlen(response) || strcmp(recv_buf, response) != 0) {
        printf("FAIL: sn_tcp_receive response (got: %s)\n", recv_buf);
        sn_tcp_close(&server_client);
        sn_tcp_close(&client);
        sn_tcp_close(&listener);
        sn_network_deinit();
        return 1;
    }

    sn_tcp_close(&server_client);
    sn_tcp_close(&client);
    sn_tcp_close(&listener);
    sn_network_deinit();

    printf("TCP TEST PASSED\n");
    return 0;
}
