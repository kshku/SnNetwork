#pragma once

#include "snnetwork/address.h"

/**
 * @file socket.h
 * @brief Opaque socket handle shared by all SnNetwork transports.
 */

/**
 * @brief Opaque socket handle.
 *
 * SnNetwork transports (TCP, UDP) all operate on the same underlying platform
 * object: a socket descriptor. This type holds that descriptor as opaque bytes;
 * the descriptor width differs between platforms (an @c int on POSIX, a
 * @c SOCKET on Windows) but is never exposed. @c SnTcpSocket and @c SnUdpSocket
 * are aliases of this type so that operations that do not depend on the
 * transport (waiting, querying the bound endpoint) can accept either.
 *
 * A socket must be closed with the transport's close function when no longer
 * needed.
 *
 * @note Thread safety: distinct sockets may be used from different threads
 *       concurrently. A single socket must not be used from two threads at the
 *       same time. SnNetwork performs no internal locking; synchronisation is
 *       the caller's responsibility.
 */
typedef struct SnSocket {
    alignas(16) uint8_t data[32];
} SnSocket;

/**
 * @brief Timeout value that makes a call block until it can complete.
 *
 * Pass this wherever a @c uint64_t @c timeout_ms parameter is accepted. It
 * mirrors the POSIX convention of a negative @c poll() timeout, expressed as an
 * unsigned sentinel. A timeout of @c 0 means "return immediately" and therefore
 * does @em not mean "block forever" (unlike the socket-level
 * sn_tcp_set_timeout()/sn_udp_set_timeout() options, which map @c 0 to no
 * @c SO_RCVTIMEO).
 */
#define SN_TIMEOUT_INFINITE UINT64_MAX

/**
 * @brief Outcome of a receive operation.
 *
 * Reported alongside the byte count so a caller can tell "the peer is slow"
 * from "the peer is gone" from "the socket broke".
 */
typedef enum SnRecvStatus {
    SN_RECV_DATA, /**< One or more bytes were read; the return is the count. */
    SN_RECV_CLOSED, /**< The peer closed the connection cleanly (return is 0). */
    SN_RECV_TIMEOUT, /**< The receive timeout elapsed with no data (return is -1). */
    SN_RECV_ERROR, /**< A fatal socket error occurred (return is -1). */
} SnRecvStatus;

/**
 * @brief One socket in an sn_socket_wait() set.
 *
 * The @c readable and @c hangup fields are outputs, populated by
 * sn_socket_wait() for every entry in the set.
 */
typedef struct SnPollFd {
    SnSocket *sock; /**< in: socket to watch (listening or connected). */
    bool readable; /**< out: data/accept pending, or the socket hung up. */
    bool hangup; /**< out: the peer closed or the socket errored. */
} SnPollFd;

/**
 * @brief Outcome of an sn_socket_wait() call.
 */
typedef enum SnWaitStatus {
    SN_WAIT_READY, /**< At least one descriptor became readable. */
    SN_WAIT_TIMEOUT, /**< The timeout elapsed with nothing readable. */
    SN_WAIT_ERROR, /**< The underlying poll failed. */
} SnWaitStatus;

/**
 * @brief Wait until any socket in a set is readable, or a deadline elapses.
 *
 * A thin, portable wrapper over @c poll() / @c WSAPoll() that hides the POSIX
 * @c int fd versus Win32 @c SOCKET difference. Works for both listening and
 * connected sockets, and for TCP and UDP alike, so a caller can wait on a mixed
 * set with a single shared deadline.
 *
 * On return every entry's @c readable and @c hangup fields are overwritten.
 *
 * @par Scratch memory
 * Sets of 64 or fewer descriptors are staged in a fixed stack buffer and never
 * allocate. A larger set is staged through the allocator installed with
 * sn_network_set_allocator(); with no allocator installed such a call fails
 * with SN_WAIT_ERROR.
 *
 * @param[in,out] fds        Array of descriptors. Every entry must hold a valid
 *                           socket. May be NULL only when @p count is 0.
 * @param[in]     count      Number of descriptors in @p fds.
 * @param[in]     timeout_ms Deadline in milliseconds. Pass 0 to poll without
 *                           blocking, or SN_TIMEOUT_INFINITE to block.
 *
 * @return SN_WAIT_READY if at least one socket is readable,
 *         SN_WAIT_TIMEOUT if the deadline elapsed, or
 *         SN_WAIT_ERROR if the wait itself failed, which includes asking for
 *         more than 64 descriptors with no allocator installed.
 */
SN_NETWORK_API SnWaitStatus sn_socket_wait(SnPollFd *fds, size_t count, uint64_t timeout_ms);

/**
 * @brief Get the local (bound) endpoint of a socket.
 *
 * Useful when a socket was bound to port 0 and the OS chose the port: this
 * reads it back so peers can be told where to connect. Works for listening,
 * connected, and bound UDP sockets.
 *
 * @param[in]  sock Socket to query.
 * @param[out] ep   Endpoint to populate with the local address and port.
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_socket_get_local_end_point(const SnSocket *sock, SnEndPoint *ep);
