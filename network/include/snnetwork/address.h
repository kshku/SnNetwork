#pragma once

#include "snnetwork/api.h"

#include <sncore/defines.h>

/**
 * @brief Fixed buffer size for sn_end_point_to_string() output.
 *
 * The longest representation is an IPv6 address with port:
 * "[ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff]:65535" = 48 characters
 * plus null terminator. 64 bytes provides ample headroom.
 *
 * All to_string output buffers must be at least this size.
 */
#define SN_ENDPOINT_STRING_MAX_LENGTH 64

/**
 * @brief IP address family discriminator.
 */
typedef enum SnAddressFamily {
    SN_ADDRESS_FAMILY_UNSPEC = 0,
    SN_ADDRESS_FAMILY_IPV4,
    SN_ADDRESS_FAMILY_IPV6,
} SnAddressFamily;

/**
 * @brief Opaque network endpoint (IP address + port).
 *
 * Can hold either an IPv4 or an IPv6 address together with a port number.
 * The internal representation is platform-specific; use the provided accessor
 * and mutator functions to inspect or construct endpoints.
 */
typedef struct SnEndPoint {
    alignas(16) uint8_t data[32];
} SnEndPoint;

/**
 * @brief Create an IPv4 endpoint from raw address bytes.
 *
 * Stores the address and port into the endpoint. The address bytes should be
 * in network byte order (big-endian), while the port is given in host byte
 * order and will be converted internally.
 *
 * @param[out] ep    Endpoint to populate.
 * @param[in]  addr  4-byte IPv4 address in network byte order.
 * @param[in]  port  Port number in host byte order.
 */
SN_NETWORK_API void sn_end_point_create_ipv4(SnEndPoint *ep, const uint8_t addr[4], uint16_t port);

/**
 * @brief Create an IPv6 endpoint from raw address bytes.
 *
 * @param[out] ep       Endpoint to populate.
 * @param[in]  addr     16-byte IPv6 address in network byte order.
 * @param[in]  port     Port number in host byte order.
 * @param[in]  scope_id Scope ID for link-local addresses (e.g. interface index).
 *                      Pass 0 when not needed.
 */
SN_NETWORK_API void sn_end_point_create_ipv6(SnEndPoint *ep, const uint8_t addr[16], uint16_t port, uint32_t scope_id);

/**
 * @brief Parse a string into an endpoint.
 *
 * Supported string formats:
 *   - "192.168.1.1:8080"       (IPv4 with port, port is required)
 *   - "[::1]:53"               (IPv6 with port)
 *   - "[fe80::1%eth0]:1234"    (IPv6 with scope and port)
 *
 * Port numbers are always required in the string.
 *
 * @param[out] ep   Parsed endpoint.
 * @param[in]  str  Null-terminated input string.
 *
 * @return true on success, false on parse failure.
 */
SN_NETWORK_API bool sn_end_point_from_string(SnEndPoint *ep, const char *str);

/**
 * @brief Get the port number from an endpoint.
 *
 * @param[in] ep  Endpoint to query.
 *
 * @return Port number in host byte order. Returns 0 if ep is NULL or uninitialized.
 */
SN_NETWORK_API uint16_t sn_end_point_get_port(const SnEndPoint *ep);

/**
 * @brief Get the address family of an endpoint.
 *
 * @param[in] ep  Endpoint to query.
 *
 * @return SN_ADDRESS_FAMILY_IPV4, SN_ADDRESS_FAMILY_IPV6, or
 *         SN_ADDRESS_FAMILY_UNSPEC if ep is NULL.
 */
SN_NETWORK_API SnAddressFamily sn_end_point_get_family(const SnEndPoint *ep);

/**
 * @brief Convert an endpoint to its string representation.
 *
 * IPv4 endpoints produce "1.2.3.4:65535" format.
 * IPv6 endpoints produce "[::1]:65535" format (with brackets).
 *
 * The buffer must be at least SN_ENDPOINT_STRING_MAX_LENGTH bytes.
 *
 * @param[in]  ep  Endpoint to format.
 * @param[out] buf Output buffer with SN_ENDPOINT_STRING_MAX_LENGTH capacity.
 */
SN_NETWORK_API void sn_end_point_to_string(const SnEndPoint *ep, char buf[SN_ENDPOINT_STRING_MAX_LENGTH]);

/**
 * @brief Compare two endpoints for equality.
 *
 * Two endpoints are equal if they have the same address family, address bytes,
 * and port number.
 *
 * @param[in] a  First endpoint.
 * @param[in] b  Second endpoint.
 *
 * @return true if the endpoints are identical, false otherwise.
 */
SN_NETWORK_API bool sn_end_point_equal(const SnEndPoint *a, const SnEndPoint *b);
