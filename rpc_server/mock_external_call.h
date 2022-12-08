#pragma once

#include "config.h"

#if USE_RPC
#error "should not include mock if rpcs enabled"
#endif

namespace scs {
struct ExternalCall
{
    struct Service
    {};
};
} // namespace scs
