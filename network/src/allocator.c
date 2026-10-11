#include "internal.h"
#include "snnetwork/network.h"

/* The library-wide allocator used for scratch memory, installed once via
   sn_network_set_allocator() and read by sn_socket_wait(). SnNetwork performs
   no locking, so this is a plain pointer set before use. */
static const SnMemoryAllocator *network_allocator;

void sn_network_set_allocator(const SnMemoryAllocator *allocator) {
    network_allocator = allocator;
}

const SnMemoryAllocator *sn_network_get_allocator(void) {
    return network_allocator;
}
