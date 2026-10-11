#pragma once

#include "snnetwork/socket.h"

/**
 * @brief Opaque TCP socket handle.
 *
 * Represents either a listening server socket or a connected client socket.
 * An alias of SnSocket: TCP and UDP sockets share the same underlying
 * representation, the name only documents intent at the call site. All
 * operations are blocking by default.
 *
 * Use the accessor functions to operate on the socket. A socket must be
 * closed with sn_tcp_close() when no longer needed.
 *
 * @note Thread safety: distinct sockets may be used from different threads
 *       concurrently, but a single socket must not be used from two threads at
 *       once. SnNetwork performs no internal locking.
 */
typedef SnSocket SnTcpSocket;

/**
 * @brief Outcome of an sn_tcp_accept() call.
 */
typedef enum SnAcceptStatus {
    SN_ACCEPT_OK, /**< A client connected; @p client is valid. */
    SN_ACCEPT_TIMEOUT, /**< No pending connection within the deadline. */
    SN_ACCEPT_ERROR, /**< A fatal error occurred on the listener. */
} SnAcceptStatus;

/**
 * @brief Direction to shut down on a connected TCP socket.
 */
typedef enum SnShutdownHow {
    SN_SHUTDOWN_RD, /**< Disallow further receives. */
    SN_SHUTDOWN_WR, /**< Disallow further sends (sends a FIN). */
    SN_SHUTDOWN_BOTH, /**< Disallow both sends and receives. */
} SnShutdownHow;

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
 * @brief Accept an incoming connection, up to a deadline.
 *
 * Waits until a new connection arrives or the deadline elapses. The newly
 * connected client socket is written to @p client, and the remote address is
 * written to @p client_ep.
 *
 * @param[in]  sock       Listening socket (initialised with sn_tcp_listen()).
 * @param[out] client     Client socket handle to populate on success.
 * @param[out] client_ep  Remote endpoint of the connected client.
 *                        Pass NULL if the remote address is not needed.
 * @param[in]  timeout_ms Deadline in milliseconds. Pass 0 for a non-blocking
 *                        accept, or SN_TIMEOUT_INFINITE to block.
 *
 * @return SN_ACCEPT_OK on success, SN_ACCEPT_TIMEOUT if no connection arrived
 *         in time, or SN_ACCEPT_ERROR on failure.
 */
SN_NETWORK_API SnAcceptStatus sn_tcp_accept(
    SnTcpSocket *sock, SnTcpSocket *client, SnEndPoint *client_ep, uint64_t timeout_ms);

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
 * @param[in]  sock   Connected socket.
 * @param[out] buf    Buffer to receive data into.
 * @param[in]  size   Capacity of the buffer in bytes.
 * @param[out] status Outcome of the call. May be NULL if the caller only
 *                    needs the byte count, but the count alone cannot tell a
 *                    timeout (-1) from an error (-1); pass a status to
 *                    distinguish them. See SnRecvStatus.
 *
 * @return The number of bytes received (status SN_RECV_DATA), 0 if the peer
 *         closed the connection (SN_RECV_CLOSED), or -1 on timeout
 *         (SN_RECV_TIMEOUT) or failure (SN_RECV_ERROR).
 */
SN_NETWORK_API int64_t sn_tcp_receive(SnTcpSocket *sock, void *buf, uint64_t size, SnRecvStatus *status);

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
 * @brief Shut down part or all of a connected TCP socket.
 *
 * Unlike sn_tcp_close(), this does not release the descriptor: it only
 * disallows further sends and/or receives. Shutting down the write direction
 * sends a FIN, which lets a peer observe a clean end of stream while this side
 * keeps draining. The socket must still be released with sn_tcp_close().
 *
 * @param[in] sock Socket to shut down.
 * @param[in] how  Direction(s) to shut down. See SnShutdownHow.
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_tcp_shutdown(SnTcpSocket *sock, SnShutdownHow how);

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
