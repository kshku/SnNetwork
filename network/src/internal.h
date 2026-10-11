#pragma once

#include <sncore/platform.h>
#include <sncore/types.h>
#include <stdbool.h>

/* Library-private accessor for the allocator installed with
   sn_network_set_allocator(). Shared between the platform sources; not part of
   the public API. Returns NULL when no allocator is installed. */
const SnMemoryAllocator *sn_network_get_allocator(void);

/* Mark a freshly created native socket handle close-on-exec (POSIX) or
   non-inheritable (Windows), so it cannot leak into a child process. The handle
   is the platform native one: an int fd on POSIX, a SOCKET on Windows. Shared
   between the platform sources; not part of the public API. Returns true when
   the handle is now non-inheritable. */
bool sn_network_set_close_on_exec(intptr_t handle);
