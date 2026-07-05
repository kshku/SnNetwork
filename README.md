# SnNetwork

Socket and networking abstraction library written in C17.

Provides TCP and UDP socket APIs with network endpoint abstraction, supporting
IPv4 and IPv6 across platforms.

## API

### Network Lifecycle (`<snnetwork/network.h>`)

| Function | Description |
|----------|-------------|
| `bool sn_network_init(bool enable_ipv6)` | Initialize networking subsystem (call once before any other networking function) |
| `void sn_network_deinit(void)` | Shutdown networking subsystem |

### Endpoint (`<snnetwork/address.h>`)

| Function | Description |
|----------|-------------|
| `void sn_end_point_create_ipv4(SnEndPoint *ep, const uint8_t addr[4], uint16_t port)` | Create an IPv4 endpoint |
| `void sn_end_point_create_ipv6(SnEndPoint *ep, const uint8_t addr[16], uint16_t port, uint32_t scope_id)` | Create an IPv6 endpoint |
| `bool sn_end_point_from_string(SnEndPoint *ep, const char *str)` | Parse "ip:port" or "[ip6]:port" string |
| `void sn_end_point_to_string(const SnEndPoint *ep, char buf[SN_ENDPOINT_STRING_MAX_LENGTH])` | Format endpoint as string |
| `uint16_t sn_end_point_get_port(const SnEndPoint *ep)` | Get port in host byte order |
| `SnAddressFamily sn_end_point_get_family(const SnEndPoint *ep)` | Get address family |
| `bool sn_end_point_equal(const SnEndPoint *a, const SnEndPoint *b)` | Compare two endpoints |

### TCP Socket (`<snnetwork/tcp.h>`)

| Function | Description |
|----------|-------------|
| `bool sn_tcp_listen(SnTcpSocket *sock, const SnEndPoint *ep, int backlog)` | Create socket, bind, and listen |
| `bool sn_tcp_accept(SnTcpSocket *sock, SnTcpSocket *client, SnEndPoint *client_ep)` | Accept a connection |
| `bool sn_tcp_connect(SnTcpSocket *sock, const SnEndPoint *ep)` | Connect to a remote endpoint |
| `int64_t sn_tcp_send(SnTcpSocket *sock, const void *data, uint64_t size)` | Send data |
| `int64_t sn_tcp_receive(SnTcpSocket *sock, void *buf, uint64_t size)` | Receive data |
| `void sn_tcp_close(SnTcpSocket *sock)` | Close socket |
| `bool sn_tcp_set_nodelay(SnTcpSocket *sock, bool enable)` | Enable/disable Nagle's algorithm |
| `bool sn_tcp_set_timeout(SnTcpSocket *sock, uint64_t timeout_ms)` | Set send/receive timeout |
| `bool sn_tcp_set_keepalive(SnTcpSocket *sock, bool enable)` | Enable/disable TCP keepalive |

### UDP Socket (`<snnetwork/udp.h>`)

| Function | Description |
|----------|-------------|
| `bool sn_udp_open(SnUdpSocket *sock)` | Create a UDP socket |
| `bool sn_udp_bind(SnUdpSocket *sock, const SnEndPoint *ep)` | Bind to a local endpoint |
| `int64_t sn_udp_send_to(SnUdpSocket *sock, const void *data, uint64_t size, const SnEndPoint *ep)` | Send data to a remote endpoint |
| `int64_t sn_udp_receive_from(SnUdpSocket *sock, void *buf, uint64_t size, SnEndPoint *sender_ep)` | Receive data and capture sender endpoint |
| `void sn_udp_close(SnUdpSocket *sock)` | Close socket |
| `bool sn_udp_set_timeout(SnUdpSocket *sock, uint64_t timeout_ms)` | Set receive timeout |

## Usage

```c
#include <snnetwork/network.h>
#include <snnetwork/tcp.h>
#include <stdio.h>
#include <string.h>

int main(void) {
    sn_network_init(false);

    SnEndPoint ep;
    uint8_t addr[4] = {127, 0, 0, 1};
    sn_end_point_create_ipv4(&ep, addr, 8080);

    SnTcpSocket listener;
    sn_tcp_listen(&listener, &ep, 1);

    SnTcpSocket client;
    sn_tcp_connect(&client, &ep);

    SnTcpSocket server;
    sn_tcp_accept(&listener, &server, NULL);

    sn_tcp_send(&client, "hello", 5);

    char buf[64] = {0};
    sn_tcp_receive(&server, buf, sizeof(buf));
    printf("Received: %s\n", buf);  // "hello"

    sn_tcp_close(&server);
    sn_tcp_close(&client);
    sn_tcp_close(&listener);
    sn_network_deinit();
    return 0;
}
```

## Adding to your project

```cmake
include(FetchContent)
FetchContent_Declare(snnetwork
    GIT_REPOSITORY https://github.com/kshku/SnNetwork.git
    GIT_TAG <tag>  # e.g., v0.1.0
)
FetchContent_MakeAvailable(snnetwork)

target_link_libraries(myapp PRIVATE snnetwork)
```

## Build

```sh
cmake -B build
cmake --build build
```

| Option | Default | Description |
|--------|---------|-------------|
| `SN_NETWORK_BUILD_SHARED` | `OFF` | Build as shared library |
| `SN_NETWORK_BUILD_TEST` | `OFF` | Build tests |

## Platform Support

| Platform | Backend |
|----------|---------|
| Linux | POSIX sockets (socket, bind, connect, etc.) |
| macOS | POSIX sockets (socket, bind, connect, etc.) |
| Windows | Win32 sockets (Winsock2) |

## Dependencies

- **SnCore** — fetched automatically via FetchContent
