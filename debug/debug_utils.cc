#include "debug/debug_utils.h"

#include "mtt/trie/debug_macros.h"

namespace debug {

std::string
storage_object_to_str(std::optional<scs::StorageObject> const& obj)
{
    if (!obj) {
        return "[nullopt]";
    }
    return storage_object_to_str(*obj);
}

std::string
storage_object_to_str(scs::StorageObject const& obj)
{
    switch (obj.body.type()) {
        case scs::ObjectType::RAW_MEMORY:
            return "[raw_memory: "
                   + debug::array_to_str(obj.body.raw_memory_storage().data) + "]";
        case scs::ObjectType::NONNEGATIVE_INT64:
            return "[nn_int64: " + std::to_string(obj.body.nonnegative_int64())
                   + "]";
        case scs::ObjectType::HASH_SET:
        {
            std::string out = "hash_set: max_size = ";
            out += std::to_string(obj.body.hash_set().max_size);
            for (auto const& h : obj.body.hash_set().hashes)
            {
                out += " <" + array_to_str(h.hash.data(), h.hash.size()) + " " + std::to_string(h.index) + ">";
            }
            return out + "]";
        }
    }
    throw std::runtime_error("unknown object type in storage_object_to_str");
}

std::string
array_to_str(const unsigned char* array, const int len)
{
    std::stringstream s;
    s.fill('0');
    for (int i = 0; i < len; i++) {
        s << std::setw(2) << std::hex << (unsigned short)array[i];
    }
    return s.str();
}

} // namespace debug
