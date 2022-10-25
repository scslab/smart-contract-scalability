#include "object/object_defaults.h"

#include <cstring>

namespace scs
{

std::optional<ObjectType>
object_type_from_delta_type(DeltaType d_type)
{
	switch(d_type)
	{
		case DeltaType::DELETE_LAST:
			return std::nullopt;
		case DeltaType::RAW_MEMORY_WRITE:
			return ObjectType::RAW_MEMORY;
		case DeltaType::NONNEGATIVE_INT64_SET_ADD:
			return ObjectType::NONNEGATIVE_INT64;
	}
	throw std::runtime_error("unimplemented object_from_delta_type");
}

std::optional<StorageObject>
make_object_from_delta(StorageDelta const& d)
{
	auto d_type = d.type();

	if (d_type == DeltaType::DELETE_LAST)
	{
		return std::nullopt;
	}

	ObjectType type;
	switch (d_type) {
		case DeltaType::RAW_MEMORY_WRITE:
			type = ObjectType::RAW_MEMORY;
			break;
		case DeltaType::NONNEGATIVE_INT64_SET_ADD:
			type = ObjectType::NONNEGATIVE_INT64;
			break;
		default:
			throw std::runtime_error(std::string("invalid type passed to make_default_object: ") + std::to_string(d_type));
	}

	StorageObject out;
	out.body.type(type);
	switch(d_type)
	{
		case DeltaType::RAW_MEMORY_WRITE:
			out.body.raw_memory_storage().data = d.data();
			break;
		case DeltaType::NONNEGATIVE_INT64_SET_ADD:
		{
			out.body.nonnegative_int64() = d.set_add_nonnegative_int64().set_value;
			int64_t delta = d.set_add_nonnegative_int64().delta;

			if (__builtin_add_overflow_p(delta, out.body.nonnegative_int64(), static_cast<int64_t>(0)))
			{
				if (delta < 0)
				{
					out.body.nonnegative_int64() = INT64_MIN;
				}
				else
				{
					out.body.nonnegative_int64() = INT64_MAX;
				}
			} else
			{
				out.body.nonnegative_int64() += delta;
			}

			break;
		}
		default:
			throw std::runtime_error("unknown d_type in make_default_object");
	}
	return out;
}

StorageObject
object_from_delta_class(StorageDeltaClass const& dc, std::optional<StorageObject> const& prev_object)
{
	StorageObject out;
	out.body.type(dc.type());

	if (prev_object)
	{
		if (prev_object -> body.type() != out.body.type())
		{
			throw std::runtime_error("type mismatch in object_from_delta_class");
		}
	}
	switch(out.body.type())
	{
		case ObjectType::RAW_MEMORY:
		{
			out.body.raw_memory_storage().data = dc.data();
			break;
		}
		case ObjectType::NONNEGATIVE_INT64:
		{
			out.body.nonnegative_int64() = dc.nonnegative_int64();
			break;
		}
		case ObjectType::HASH_SET:
		{
			out.body.hash_set().max_size = START_HASH_SET_SIZE;

			if (prev_object)
			{
				out.body.hash_set().max_size = prev_object -> body.hash_set().max_size;
				out.body.hash_set().hashes = prev_object -> body.hash_set().hashes;
			}
			break;
		}
		default:
			throw std::runtime_error("unimpl sdc object_defaults.cc");
	}
	return out;
}
/*
StorageObject
make_default_object_by_type(ObjectType type)
{
	StorageObject out;
	out.type(type);
	switch(type)
	{
		case ObjectType::RAW_MEMORY:
			// default is empty data field
			break;
		case ObjectType::NONNEGATIVE_INT64:
			out.nonnegative_int64() = 0;
			break;
		default:
			throw std::runtime_error("unknown object type");
			
	}
	return out;
} */

} /* scs */
