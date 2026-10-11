#pragma once

/**
 * @file network.h
 * @brief Umbrella header for the SnNetwork library.
 *
 * Includes all public API headers of the networking abstraction library.
 * Include this single header to gain access to all types and functions, or
 * include individual headers (address.h, socket.h, tcp.h, udp.h) for finer
 * granularity.
 */

#include "snnetwork/address.h"
#include "snnetwork/api.h"
#include "snnetwork/socket.h"
#include "snnetwork/tcp.h"
#include "snnetwork/udp.h"

#include <sncore/defines.h>
#include <sncore/types.h>

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

/**
 * @brief Install the allocator SnNetwork uses for its scratch memory.
 *
 * SnNetwork never hides a heap allocation. The one place it may need memory it
 * does not already own is sn_socket_wait(): a set larger than the built-in
 * stack scratch is staged in a platform-native poll array. This setter supplies
 * the allocator for that array.
 *
 * @par Where it is used
 * Only sn_socket_wait() reads this allocator, and only when the set holds more
 * than 64 sockets. Sets of 64 or fewer are staged on the stack and never touch
 * the allocator, so they work with no allocator installed.
 *
 * @par When you must provide one
 * Calling sn_socket_wait() with more than 64 sockets while no allocator is
 * installed returns SN_WAIT_ERROR. Install one only if your program waits on
 * larger sets, for example the @c sn_std_allocator that SnMemory provides, or
 * any SnMemory allocator backed by caller-owned memory.
 *
 * @par Lifetime
 * The block is requested at the start of the call and freed before it returns;
 * SnNetwork never retains it. The allocator itself is borrowed, so it must
 * outlive every call that uses it.
 *
 * @param[in] allocator Allocator to use, or NULL to clear the current one.
 *
 * @note Not thread-safe. Set it once, before the first sn_socket_wait() that
 *       needs it, alongside sn_network_init().
 */
SN_NETWORK_API void sn_network_set_allocator(const SnMemoryAllocator *allocator);
