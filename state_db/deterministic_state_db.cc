#include "state_db/deterministic_state_db.h"

namespace scs
{

std::optional<StorageObject>
DeterministicStateDB::get_committed_value(const AddressAndKey& a)
{
    auto const* res = state_db.get_value(a);
    if (res) {
        return (res)->object.get_committed_object();
    }
    return std::nullopt;
}

void
do_nothing_if_merge(const DeterministicStateDB::value_t& value)
{}

void
DeterministicStateDB::log_typeclass(const AddressAndKey& a,
                                 	const PrioritizedStorageDelta& delta)
{
    auto* res = state_db.get_value(a, true);

    if (!res) {
        auto* root = state_db.get_root_and_invalidate_hash(current_timestamp);

        root->template insert<&do_nothing_if_merge>(a, state_db.get_gc(), current_timestamp, state_db.get_storage());

        res = state_db.get_value(a, true);
    }

    if (res == nullptr) {
        throw std::runtime_error("should be impossible");
    }

    res -> tc.log_prioritized_typeclass(delta);
}



}
