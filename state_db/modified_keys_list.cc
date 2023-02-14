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

#include "state_db/modified_keys_list.h"

#include "debug/debug_utils.h"

namespace scs {

void
ModifiedKeysList::assert_logs_merged() const
{
    if (!logs_merged) {
        throw std::runtime_error("logs not merged");
    }
}
void
ModifiedKeysList::assert_logs_not_merged() const
{
    if (logs_merged) {
        throw std::runtime_error("logs already merged");
    }
}

void
ModifiedKeysList::log_key(const AddressAndKey& key)
{
    assert_logs_not_merged();
    auto& local_trie = cache.get(keys);
    local_trie.insert(key);
}

void
ModifiedKeysList::merge_logs()
{
    assert_logs_not_merged();
   // keys.template batch_merge_in<trie::OverwriteMergeFn>(cache);

    logs_merged = true;
}

Hash
ModifiedKeysList::hash()
{
    assert_logs_merged();
    Hash out;
    auto h = keys.hash();
    std::memcpy(out.data(), h.data(), h.size());

    return out;
}

void 
ModifiedKeysList::clear()
{
    cache.clear();
    keys.clear();
    logs_merged = false;
}

} // namespace scs