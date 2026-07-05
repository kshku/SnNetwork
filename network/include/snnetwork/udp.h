#pragma once

#include "snnetwork/address.h"

/**
 * @brief Opaque UDP socket handle.
 *
 * Represents a connectionless UDP socket that can send and receive datagrams.
 * The underlying representation wraps a platform socket descriptor together
 * with optional state flags. All operations are blocking by default.
 *
 * A socket must be closed with sn_udp_close() when no longer needed.
 */
typedef struct SnUdpSocket {
    alignas(16) uint8_t data[32];
} SnUdpSocket;

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
 *
 * @return The number of bytes received on success, or -1 on failure.
 */
SN_NETWORK_API int64_t sn_udp_receive_from(SnUdpSocket *sock, void *buf, uint64_t size, SnEndPoint *sender_ep);

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
