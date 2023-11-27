#pragma once

#include "object/revertable_object.h"
#include "xdr/storage.h"

#include "mtt/memcached_snapshot_trie/durable_interface.h"

#include <xdrpp/marshal.h>

namespace scs {

class OptionalValueWrapper : public RevertableObject
{
    using slice_t = trie::DurableValueSlice;

    static StorageObject from_slice(const slice_t& slice)
    {
        throw std::runtime_error("unimplemented");
    }

  public:
    using RevertableObject::RevertableObject;

    OptionalValueWrapper(slice_t const& slice)
        : RevertableObject(from_slice(slice))
    {}

    void copy_data(std::vector<uint8_t>& buf) const
    {
        auto const& res = get_committed_object();
        if (res) {
            xdr::append_xdr_to_opaque(buf, *res);
        }
    }
};

} // namespace scs
