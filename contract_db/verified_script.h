#pragma once

#include <cstdint>
#include <memory>
#include <vector>

#include "crypto/hash.h"

namespace scs {

struct VerifiedScriptView
{
    const uint8_t* bytes;
    const size_t len;
};

struct VerifiedScript
{
    const std::vector<uint8_t> bytes;

    VerifiedScriptView to_view() const
    {
        return VerifiedScriptView{ .bytes = bytes.data(), .len = bytes.size() };
    }

    Hash hash() const {
        return hash_vec(bytes);
    }

    Contract to_tx_contract() {
        Contract out;
        out.insert(out.end(), bytes.begin(), bytes.end());
        return out;
    }
};

using verified_script_ptr_t = std::shared_ptr<VerifiedScript>;

} // namespace scs
