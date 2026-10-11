#pragma once

#include <sncore/types.h>

/* Library-private accessor for the allocator installed with
   sn_network_set_allocator(). Shared between the platform sources; not part of
   the public API. Returns NULL when no allocator is installed. */
const SnMemoryAllocator *sn_network_get_allocator(void);
