#include <snnetwork/network.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(SN_OS_WINDOWS)
    #include <windows.h>
#else
    #include <fcntl.h>
#endif

static const uint8_t LOOPBACK[4] = {127, 0, 0, 1};
static int failures = 0;

static void fail(const char *name) {
    printf("FAIL: %s\n", name);
    failures++;
}

static bool make_listener(SnTcpSocket *listener, uint16_t *port) {
    SnEndPoint ep;
    sn_end_point_create_ipv4(&ep, LOOPBACK, 0);
    if (!sn_tcp_listen(listener, &ep, 4)) return false;

    SnEndPoint local;
    if (!sn_socket_get_local_end_point(listener, &local)) return false;

    *port = sn_end_point_get_port(&local);
    return *port != 0;
}

static bool connect_client(SnTcpSocket *client, uint16_t port) {
    SnEndPoint ep;
    sn_end_point_create_ipv4(&ep, LOOPBACK, port);
    return sn_tcp_connect(client, &ep);
}

/* R4: a socket bound to port 0 reports the port the OS chose, and a client can
   connect to it. */
static void test_local_end_point(void) {
    SnTcpSocket listener;
    uint16_t port = 0;
    if (!make_listener(&listener, &port)) {
        fail("get_local_end_point");
        if (port == 0) sn_tcp_close(&listener);
        return;
    }

    SnTcpSocket client;
    if (!connect_client(&client, port)) {
        fail("connect to reported port");
        sn_tcp_close(&listener);
        return;
    }

    SnTcpSocket server;
    if (sn_tcp_accept(&listener, &server, NULL, 1000) != SN_ACCEPT_OK) {
        fail("accept after connecting to reported port");
    } else {
        sn_tcp_close(&server);
    }

    sn_tcp_close(&client);
    sn_tcp_close(&listener);
}

/* R1: the right descriptor in a mixed listener/connected set is reported
   readable and the others are not. */
static void test_wait_reports_the_right_fd(void) {
    SnTcpSocket listener;
    uint16_t port = 0;
    if (!make_listener(&listener, &port)) {
        fail("wait: listen");
        return;
    }

    SnTcpSocket client1, client2;
    if (!connect_client(&client1, port) || !connect_client(&client2, port)) {
        fail("wait: connect");
        sn_tcp_close(&client1);
        sn_tcp_close(&client2);
        sn_tcp_close(&listener);
        return;
    }

    SnTcpSocket server1, server2;
    if (sn_tcp_accept(&listener, &server1, NULL, 1000) != SN_ACCEPT_OK
        || sn_tcp_accept(&listener, &server2, NULL, 1000) != SN_ACCEPT_OK) {
        fail("wait: accept");
        sn_tcp_close(&client1);
        sn_tcp_close(&client2);
        sn_tcp_close(&listener);
        return;
    }

    SnPollFd fds[3];
    fds[0].sock = &listener;
    fds[1].sock = &server1;
    fds[2].sock = &server2;

    /* Nothing pending yet but the listener has no queue entry: timeout. */
    if (sn_socket_wait(fds, 3, 0) != SN_WAIT_TIMEOUT) fail("wait: idle should time out");

    /* Data on client1: server1 readable, listener and server2 not. */
    if (sn_tcp_send(&client1, "a", 1) != 1) fail("wait: send");
    if (sn_socket_wait(fds, 3, 1000) != SN_WAIT_READY) {
        fail("wait: ready");
    } else {
        if (!fds[1].readable) fail("wait: server1 readable");
        if (fds[0].readable) fail("wait: listener must not be readable");
        if (fds[2].readable) fail("wait: server2 must not be readable");
    }

    char buf[8];
    SnRecvStatus status = SN_RECV_ERROR;
    sn_tcp_receive(&server1, buf, sizeof(buf), &status);

    /* A new connection: the listener becomes readable. */
    SnTcpSocket client3;
    if (!connect_client(&client3, port)) fail("wait: connect third");
    if (sn_socket_wait(fds, 3, 1000) != SN_WAIT_READY) {
        fail("wait: listener ready");
    } else if (!fds[0].readable) {
        fail("wait: listener readable on pending connection");
    }

    SnTcpSocket server3;
    if (sn_tcp_accept(&listener, &server3, NULL, 0) == SN_ACCEPT_OK) sn_tcp_close(&server3);

    sn_tcp_close(&client1);
    sn_tcp_close(&client2);
    sn_tcp_close(&client3);
    sn_tcp_close(&server1);
    sn_tcp_close(&server2);
    sn_tcp_close(&listener);
}

/* R1: a closed peer is reported as readable + hangup. */
static void test_wait_hangup(void) {
    SnTcpSocket listener;
    uint16_t port = 0;
    if (!make_listener(&listener, &port)) {
        fail("hangup: listen");
        return;
    }

    SnTcpSocket client;
    SnTcpSocket server;
    if (!connect_client(&client, port) || sn_tcp_accept(&listener, &server, NULL, 1000) != SN_ACCEPT_OK) {
        fail("hangup: connect/accept");
        sn_tcp_close(&client);
        sn_tcp_close(&listener);
        return;
    }

    sn_tcp_close(&client);

    SnPollFd fds[1];
    fds[0].sock = &server;
    if (sn_socket_wait(fds, 1, 1000) != SN_WAIT_READY) {
        fail("hangup: wait ready");
    } else {
        if (!fds[0].readable) fail("hangup: readable");
        if (!fds[0].hangup) fail("hangup: hangup flag");
    }

    sn_tcp_close(&server);
    sn_tcp_close(&listener);
}

/* R1: a large set returns promptly, and a listening socket bound to port 0 has
   a non-zero port. */
static void test_wait_many_descriptors(void) {
    SnTcpSocket listeners[64];
    SnPollFd fds[64];
    size_t opened = 0;

    for (size_t i = 0; i < SN_ARRAY_LENGTH(listeners); ++i) {
        uint16_t port = 0;
        if (!make_listener(&listeners[i], &port)) break;
        fds[i].sock = &listeners[i];
        opened++;
    }

    if (opened != SN_ARRAY_LENGTH(listeners)) {
        fail("many: open 64 listeners");
    } else if (sn_socket_wait(fds, opened, 0) != SN_WAIT_TIMEOUT) {
        fail("many: idle set should time out immediately");
    } else {
        for (size_t i = 0; i < opened; ++i) {
            if (fds[i].readable) fail("many: no listener should be readable");
        }
    }

    for (size_t i = 0; i < opened; ++i) {
        sn_tcp_close(&listeners[i]);
    }
}

/* A set larger than the built-in stack scratch must go through the allocator
   installed with sn_network_set_allocator(), and must fail without one. */
static int alloc_calls;
static int free_calls;

static void *counting_alloc(void *data, uint64_t size, uint64_t align) {
    (void)data;
    (void)align;
    alloc_calls++;
    return malloc((size_t)size);
}

static void counting_free(void *data, void *ptr) {
    (void)data;
    free_calls++;
    free(ptr);
}

static SnMemoryAllocator counting_allocator = {
    .data = NULL,
    .alloc = counting_alloc,
    .realloc = NULL,
    .free = counting_free,
};

/* R5: a set of more than 64 descriptors needs the installed allocator. */
static void test_wait_large_set_allocator(void) {
    SnPollFd nulls[65];
    for (size_t i = 0; i < SN_ARRAY_LENGTH(nulls); ++i) nulls[i].sock = NULL;

    if (sn_socket_wait(nulls, SN_ARRAY_LENGTH(nulls), 0) != SN_WAIT_ERROR) {
        fail("large: no allocator must fail");
    }

    SnTcpSocket listeners[65];
    SnPollFd fds[65];
    size_t opened = 0;
    for (size_t i = 0; i < SN_ARRAY_LENGTH(listeners); ++i) {
        uint16_t port = 0;
        if (!make_listener(&listeners[i], &port)) break;
        fds[i].sock = &listeners[i];
        opened++;
    }

    if (opened != SN_ARRAY_LENGTH(listeners)) {
        fail("large: open 65 listeners");
    } else {
        alloc_calls = 0;
        free_calls = 0;
        sn_network_set_allocator(&counting_allocator);
        SnWaitStatus status = sn_socket_wait(fds, opened, 0);
        sn_network_set_allocator(NULL);

        if (status != SN_WAIT_TIMEOUT) fail("large: idle set should time out");
        if (alloc_calls != 1) fail("large: allocator should be used once");
        if (free_calls != 1) fail("large: scratch must be freed");
    }

    for (size_t i = 0; i < opened; ++i) {
        sn_tcp_close(&listeners[i]);
    }
}

/* R3: accept honours its deadline and never blocks when the timeout is zero. */
static void test_accept_deadline(void) {
    SnTcpSocket listener;
    uint16_t port = 0;
    if (!make_listener(&listener, &port)) {
        fail("accept: listen");
        return;
    }

    SnTcpSocket client;
    if (sn_tcp_accept(&listener, &client, NULL, 0) != SN_ACCEPT_TIMEOUT) {
        fail("accept: zero timeout must not block");
    }
    if (sn_tcp_accept(&listener, &client, NULL, 50) != SN_ACCEPT_TIMEOUT) {
        fail("accept: no pending connection should time out");
    }

    SnTcpSocket real_client;
    if (!connect_client(&real_client, port)) {
        fail("accept: connect");
        sn_tcp_close(&listener);
        return;
    }
    if (sn_tcp_accept(&listener, &client, NULL, 1000) != SN_ACCEPT_OK) {
        fail("accept: pending connection should succeed");
    } else {
        sn_tcp_close(&client);
    }

    sn_tcp_close(&real_client);
    sn_tcp_close(&listener);
}

/* R2: receive distinguishes timeout, close and data. */
static void test_receive_status(void) {
    SnTcpSocket listener;
    uint16_t port = 0;
    if (!make_listener(&listener, &port)) {
        fail("receive: listen");
        return;
    }

    SnTcpSocket client;
    SnTcpSocket server;
    if (!connect_client(&client, port) || sn_tcp_accept(&listener, &server, NULL, 1000) != SN_ACCEPT_OK) {
        fail("receive: connect/accept");
        sn_tcp_close(&client);
        sn_tcp_close(&listener);
        return;
    }

    if (!sn_tcp_set_timeout(&server, 50)) fail("receive: set timeout");

    char buf[16];
    SnRecvStatus status = SN_RECV_ERROR;
    int64_t received = sn_tcp_receive(&server, buf, sizeof(buf), &status);
    if (received != -1 || status != SN_RECV_TIMEOUT) fail("receive: timeout");

    if (sn_tcp_send(&client, "hi", 2) != 2) fail("receive: send");
    memset(buf, 0, sizeof(buf));
    received = sn_tcp_receive(&server, buf, sizeof(buf), &status);
    if (received != 2 || status != SN_RECV_DATA || strcmp(buf, "hi") != 0) fail("receive: data");

    sn_tcp_close(&client);
    received = sn_tcp_receive(&server, buf, sizeof(buf), &status);
    if (received != 0 || status != SN_RECV_CLOSED) fail("receive: closed");

    sn_tcp_close(&server);
    sn_tcp_close(&listener);
}

/* R6: shutting down the write side signals end-of-stream to the peer. */
static void test_shutdown(void) {
    SnTcpSocket listener;
    uint16_t port = 0;
    if (!make_listener(&listener, &port)) {
        fail("shutdown: listen");
        return;
    }

    SnTcpSocket client;
    SnTcpSocket server;
    if (!connect_client(&client, port) || sn_tcp_accept(&listener, &server, NULL, 1000) != SN_ACCEPT_OK) {
        fail("shutdown: connect/accept");
        sn_tcp_close(&client);
        sn_tcp_close(&listener);
        return;
    }

    if (!sn_tcp_shutdown(&client, SN_SHUTDOWN_WR)) fail("shutdown: call");

    char buf[16];
    SnRecvStatus status = SN_RECV_ERROR;
    if (sn_tcp_receive(&server, buf, sizeof(buf), &status) != 0 || status != SN_RECV_CLOSED) {
        fail("shutdown: peer sees end of stream");
    }

    sn_tcp_close(&client);
    sn_tcp_close(&server);
    sn_tcp_close(&listener);
}

/* R8: sockets must not leak into a spawned child process, so every socket
   SnNetwork creates or accepts is close-on-exec / non-inheritable. */
static bool socket_is_inheritable(const SnSocket *sock) {
#if defined(SN_OS_WINDOWS)
    uintptr_t handle = 0;
    memcpy(&handle, sock->data, sizeof(handle));
    DWORD flags = 0;
    if (!GetHandleInformation((HANDLE)handle, &flags)) return false;
    return (flags & HANDLE_FLAG_INHERIT) != 0;
#else
    int fd = -1;
    memcpy(&fd, sock->data, sizeof(fd));
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0) return false;
    return (flags & FD_CLOEXEC) == 0;
#endif
}

static void test_sockets_not_inherited(void) {
    SnTcpSocket listener;
    uint16_t port = 0;
    if (!make_listener(&listener, &port)) {
        fail("inheritance: listen");
        return;
    }

    SnTcpSocket client;
    SnTcpSocket server;
    if (!connect_client(&client, port) || sn_tcp_accept(&listener, &server, NULL, 1000) != SN_ACCEPT_OK) {
        fail("inheritance: connect/accept");
        sn_tcp_close(&listener);
        return;
    }

    if (socket_is_inheritable(&listener)) fail("inheritance: listener can leak");
    if (socket_is_inheritable(&client)) fail("inheritance: client can leak");
    if (socket_is_inheritable(&server)) fail("inheritance: accepted socket can leak");

    SnUdpSocket udp;
    if (!sn_udp_open(&udp)) {
        fail("inheritance: udp open");
    } else {
        if (socket_is_inheritable(&udp)) fail("inheritance: udp can leak");
        sn_udp_close(&udp);
    }

    sn_tcp_close(&client);
    sn_tcp_close(&server);
    sn_tcp_close(&listener);
}

int main(void) {
    if (!sn_network_init()) {
        printf("FAIL: sn_network_init\n");
        return 1;
    }

    test_local_end_point();
    test_wait_reports_the_right_fd();
    test_wait_hangup();
    test_wait_many_descriptors();
    test_wait_large_set_allocator();
    test_accept_deadline();
    test_receive_status();
    test_shutdown();
    test_sockets_not_inherited();

    sn_network_deinit();

    if (failures == 0) {
        printf("SOCKET TEST PASSED\n");
        return 0;
    }

    printf("SOCKET TEST FAILED (%d)\n", failures);
    return 1;
}
