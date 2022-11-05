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
    local_trie.insert(key, trie::EmptyValue{});
}

void
ModifiedKeysList::merge_logs()
{
    assert_logs_not_merged();
    keys.template batch_merge_in<trie::OverwriteMergeFn>(cache);

    logs_merged = true;
}

Hash
ModifiedKeysList::hash()
{
    assert_logs_merged();

    Hash out;
    keys.hash(out);
    return out;
}

} // namespace scs