/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include "mtt/ephemeral_trie/concepts.h"

#include "xdr/storage_delta.h"

#include <cstdint>
#include <variant>
#include <vector>

namespace scs {

class ModificationMetadata : public trie::EphemeralTrieMetadataBase
{
  public:
    using int128_t = __int128;

    enum ActiveMeta
    {
        NNINT64,
        HASHSET_LIMIT,
        NONE
    };

  private:
    union
    {
        int128_t nnint64_delta;
        uint64_t hashset_limit_delta;
    } meta;

    ActiveMeta active_meta;

  public:
    ActiveMeta const& get_active() const { return active_meta; }

    int128_t get_nnint64() const
    {
        if (active_meta != NNINT64) {
            throw std::runtime_error("mismatch");
        }
        return meta.nnint64_delta;
    }

    int128_t get_hs_limit() const
    {
        if (active_meta != HASHSET_LIMIT) {
            throw std::runtime_error("mismatch");
        }
        return meta.hashset_limit_delta;
    }

    void write_to(std::vector<uint8_t>& digest_bytes) const
    {
        switch (active_meta) {
            case ActiveMeta::NNINT64: {
                const uint8_t* ptr
                    = reinterpret_cast<const uint8_t*>(&meta.nnint64_delta);
                digest_bytes.insert(
                    digest_bytes.end(), ptr, ptr + sizeof(int128_t));
                break;
            }
            case ActiveMeta::HASHSET_LIMIT: {
                const uint8_t* ptr = reinterpret_cast<const uint8_t*>(
                    &meta.hashset_limit_delta);
                digest_bytes.insert(
                    digest_bytes.end(), ptr, ptr + sizeof(uint64_t));
                break;
            }
            case ActiveMeta::NONE: {
                break;
            }
            default:
                throw std::runtime_error("invalid meta enum");
        }

        EphemeralTrieMetadataBase::write_to(digest_bytes);
    }

    void from_value(StorageDelta const& value)
    {
        switch (value.type()) {
            case DeltaType::NONNEGATIVE_INT64_SET_ADD: {
                active_meta = ActiveMeta::NNINT64;
                meta.nnint64_delta = value.set_add_nonnegative_int64().delta;
                break;
            }
            case DeltaType::HASH_SET_INCREASE_LIMIT: {
                active_meta = ActiveMeta::HASHSET_LIMIT;
                meta.hashset_limit_delta = value.limit_increase();
                break;
            }
            default: {
                active_meta = ActiveMeta::NONE;
                break;
            }
        }

        EphemeralTrieMetadataBase::from_value(value);
    }

    ModificationMetadata& operator+=(const ModificationMetadata& other)
    {
        if (active_meta == ActiveMeta::NONE) {
            active_meta = other.active_meta;
            switch (active_meta) {
                case ActiveMeta::NNINT64: {
                    meta.nnint64_delta = other.meta.nnint64_delta;
                    break;
                }
                case ActiveMeta::HASHSET_LIMIT: {
                    meta.hashset_limit_delta = other.meta.hashset_limit_delta;
                    break;
                }
                case ActiveMeta::NONE: {
                    break;
                }
                default:
                    throw std::runtime_error("impossible");
            }
        } else if (other.active_meta == active_meta) {
            switch (active_meta) {
                // we're implementing the honest operator,
                // for which the additions cannot overflow (by design)
                case ActiveMeta::NNINT64: {
                    meta.nnint64_delta += other.meta.nnint64_delta;
                    break;
                }
                case ActiveMeta::HASHSET_LIMIT: {
                    meta.hashset_limit_delta += other.meta.hashset_limit_delta;
                    break;
                }
                case ActiveMeta::NONE:
                    break;
                default:
                    throw std::runtime_error("invalid active meta type");
            }
        } else {
            active_meta = ActiveMeta::NONE;
        }

        EphemeralTrieMetadataBase::operator+=(other);
        return *this;
    }

    void clear()
    {
        active_meta = ActiveMeta::NONE;
        EphemeralTrieMetadataBase::clear();
    }
};

} // namespace scs
