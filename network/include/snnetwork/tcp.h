#pragma once

#include "snnetwork/address.h"

/**
 * @brief Opaque TCP socket handle.
 *
 * Represents either a listening server socket or a connected client socket.
 * The underlying representation wraps a platform socket descriptor together
 * with optional state flags. All operations are blocking by default.
 *
 * Use the accessor functions to operate on the socket. A socket must be
 * closed with sn_tcp_close() when no longer needed.
 */
typedef struct SnTcpSocket {
    alignas(16) uint8_t data[32];
} SnTcpSocket;

/**
 * @brief Initialise and bind a socket to an endpoint for accepting connections.
 *
 * Creates a new TCP socket, binds it to the given local endpoint, and starts
 * listening for incoming connections. The endpoint includes both the address
 * to bind to and the port number.
 *
 * @param[out] sock    Socket handle to initialise.
 * @param[in]  ep      Local endpoint to bind to. Pass NULL or an
 *                     SN_ADDRESS_FAMILY_UNSPEC endpoint to let the system
 *                     choose the address.
 * @param[in]  backlog Maximum length of the pending connections queue.
 *                     Pass -1 to use the system default.
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_tcp_listen(SnTcpSocket *sock, const SnEndPoint *ep, int backlog);

/**
 * @brief Accept an incoming connection on a listening socket.
 *
 * Blocks until a new connection arrives. The newly connected client socket
 * is written to @p client, and the remote address is written to @p client_ep.
 *
 * @param[in]  sock      Listening socket (initialised with sn_tcp_listen()).
 * @param[out] client    Client socket handle to populate.
 * @param[out] client_ep Remote endpoint of the connected client.
 *                       Pass NULL if the remote address is not needed.
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_tcp_accept(SnTcpSocket *sock, SnTcpSocket *client, SnEndPoint *client_ep);

/**
 * @brief Connect to a remote TCP endpoint.
 *
 * Creates a new TCP socket and connects it to the given remote endpoint.
 * Blocks until the connection is established or fails.
 *
 * @param[out] sock  Socket handle to initialise.
 * @param[in]  ep    Remote endpoint to connect to.
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_tcp_connect(SnTcpSocket *sock, const SnEndPoint *ep);

/**
 * @brief Send data over a connected TCP socket.
 *
 * @param[in]  sock  Connected socket.
 * @param[in]  data  Pointer to the data to send.
 * @param[in]  size  Number of bytes to send.
 *
 * @return The number of bytes actually sent on success,
 *         or -1 on failure.
 */
SN_NETWORK_API int64_t sn_tcp_send(SnTcpSocket *sock, const void *data, uint64_t size);

/**
 * @brief Receive data from a connected TCP socket.
 *
 * Blocks until data is available or the connection is closed / fails.
 *
 * @param[in]  sock Connected socket.
 * @param[out] buf  Buffer to receive data into.
 * @param[in]  size Capacity of the buffer in bytes.
 *
 * @return The number of bytes received on success (zero means the remote
 *         peer has closed the connection), or -1 on failure.
 */
SN_NETWORK_API int64_t sn_tcp_receive(SnTcpSocket *sock, void *buf, uint64_t size);

/**
 * @brief Close a TCP socket and release its resources.
 *
 * After this call the socket handle must not be used again without a
 * subsequent call to sn_tcp_listen() or sn_tcp_connect().
 *
 * @param[in] sock Socket to close.
 */
SN_NETWORK_API void sn_tcp_close(SnTcpSocket *sock);

/**
 * @brief Enable or disable TCP_NODELAY on a socket.
 *
 * When enabled, no buffering is performed by the TCP stack and data is
 * sent as soon as possible (useful for interactive protocols).
 *
 * @param[in] sock   Socket to configure.
 * @param[in] enable true to enable, false to disable.
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_tcp_set_nodelay(SnTcpSocket *sock, bool enable);

/**
 * @brief Set the send and receive timeout on a socket.
 *
 * @param[in] sock       Socket to configure.
 * @param[in] timeout_ms Timeout in milliseconds. Pass 0 for no timeout
 *                       (blocking behaviour).
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_tcp_set_timeout(SnTcpSocket *sock, uint64_t timeout_ms);

/**
 * @brief Enable or disable TCP keepalive on a socket.
 *
 * When enabled, the TCP stack periodically sends keepalive probes to
 * verify that the remote peer is still reachable.
 *
 * @param[in] sock   Socket to configure.
 * @param[in] enable true to enable, false to disable.
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_tcp_set_keepalive(SnTcpSocket *sock, bool enable);
