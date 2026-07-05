#pragma once

/**
 * @file network.h
 * @brief Umbrella header for the SnNetwork library.
 *
 * Includes all public API headers of the networking abstraction library.
 * Include this single header to gain access to all types and functions, or
 * include individual headers (address.h, tcp.h, udp.h) for finer granularity.
 */

#include "snnetwork/api.h"
#include "snnetwork/address.h"
#include "snnetwork/tcp.h"
#include "snnetwork/udp.h"

#include <sncore/defines.h>

/**
 * @brief Initialise the network subsystem.
 *
 * On Windows this internally calls WSAStartup(). On POSIX platforms this is
 * a no-op. Must be called once before any other SnNetwork function.
 *
 * @return true on success, false on failure.
 */
SN_NETWORK_API bool sn_network_init(void);

/**
 * @brief Shut down the network subsystem.
 *
 * On Windows this internally calls WSACleanup(). On POSIX platforms this is
 * a no-op. Must be called after all network operations are finished.
 */
SN_NETWORK_API void sn_network_deinit(void);
