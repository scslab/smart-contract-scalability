#include "state_db/modified_keys_list.h"

namespace scs {

void
ModifiedKeysList::assert_logs_merged()
{
    if (!logs_merged) {
        throw std::runtime_error("logs not merged");
    }
}
void
ModifiedKeysList::assert_logs_not_merged()
{
    if (logs_merged) {
        throw std::runtime_error("logs already merged");
    }
}

void
ModifiedKeysList::log_key(const AddressAndKey& key)
{
    assert_not_logs_merged();
    auto& local_trie = cache.get(keys);
    local_trie.insert(key, trie::EmptyValue{});
}

void
ModifiedKeysList::merge_logs()
{
    assert_not_logs_merged();
    keys.template batch_merge_in<trie::OverwriteMergeFn>(cache);
    logs_merged = true;
}

} // namespace scs