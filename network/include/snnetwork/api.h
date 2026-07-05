#pragma once

#include <sncore/api_common.h>
#if defined(SN_NETWORK_STATIC)
    #define SN_NETWORK_API
#elif defined(SN_EXPORT)
    #define SN_NETWORK_API SN_API_HELPER_EXPORT
#else
    #define SN_NETWORK_API SN_API_HELPER_IMPORT
#endif
