#pragma once

#include <cstdint>
#include <vector>

namespace scs {

struct TransactionResults
{
    std::vector<std::vector<uint8_t>> logs;
};

} // namespace scs