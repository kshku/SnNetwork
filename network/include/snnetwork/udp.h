#pragma once

#include "snnetwork/socket.h"

/**
 * @brief Opaque UDP socket handle.
 *
 * Represents a connectionless UDP socket that can send and receive datagrams.
 * An alias of SnSocket: TCP and UDP sockets share the same underlying
 * representation, the name only documents intent at the call site. All
 * operations are blocking by default.
 *
 * A socket must be closed with sn_udp_close() when no longer needed.
 *
 * @note Thread safety: distinct sockets may be used from different threads
 *       concurrently, but a single socket must not be used from two threads at
 *       once. SnNetwork performs no internal locking.
 */
typedef SnSocket SnUdpSocket;

/**
 * @brief Open a new UDP socket.
 *
 * Creates a socket in the AF_INET or AF_INET6 domain. By default the socket
 * is not bound; call sn_udp_bind() to bind it to a local endpoint if the
 * socket needs to receive datagrams.
 *
 * @param[out] sock Socket handle to initialise.
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_udp_open(SnUdpSocket *sock);

/**
 * @brief Bind a UDP socket to a local endpoint.
 *
 * After binding, the socket is ready to receive datagrams addressed to the
 * given endpoint. Call sn_udp_open() first to create the socket.
 *
 * @param[in] sock Socket to bind.
 * @param[in] ep   Local endpoint to bind to (address + port).
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_udp_bind(SnUdpSocket *sock, const SnEndPoint *ep);

/**
 * @brief Send a datagram to a remote endpoint.
 *
 * @param[in] sock Socket to send from.
 * @param[in] data Pointer to the datagram data.
 * @param[in] size Number of bytes to send (must fit in a single UDP datagram).
 * @param[in] ep   Remote endpoint to send to.
 *
 * @return The number of bytes sent on success, or -1 on failure.
 */
SN_NETWORK_API int64_t sn_udp_send_to(SnUdpSocket *sock, const void *data, uint64_t size, const SnEndPoint *ep);

/**
 * @brief Receive a datagram and record the sender's address.
 *
 * Blocks until a datagram arrives.
 *
 * @param[in]  sock      Socket to receive on.
 * @param[out] buf       Buffer to receive the datagram data.
 * @param[in]  size      Capacity of the buffer in bytes.
 * @param[out] sender_ep Sender's endpoint (address + port).
 *                       Pass NULL if the sender address is not needed.
 * @param[out] status    Outcome of the call. May be NULL if the caller only
 *                       needs the byte count, but the count alone cannot tell
 *                       a timeout (-1) from an error (-1); pass a status to
 *                       distinguish them. See SnRecvStatus.
 *
 * @return The number of bytes received (status SN_RECV_DATA), or -1 on timeout
 *         (SN_RECV_TIMEOUT) or failure (SN_RECV_ERROR).
 */
SN_NETWORK_API int64_t sn_udp_receive_from(
    SnUdpSocket *sock, void *buf, uint64_t size, SnEndPoint *sender_ep, SnRecvStatus *status);

/**
 * @brief Close a UDP socket and release its resources.
 *
 * After this call the socket handle must not be used again without a
 * subsequent call to sn_udp_open().
 *
 * @param[in] sock Socket to close.
 */
SN_NETWORK_API void sn_udp_close(SnUdpSocket *sock);

/**
 * @brief Set the receive timeout on a UDP socket.
 *
 * @param[in] sock       Socket to configure.
 * @param[in] timeout_ms Timeout in milliseconds. Pass 0 for no timeout
 *                       (blocking behaviour).
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_udp_set_timeout(SnUdpSocket *sock, uint64_t timeout_ms);
