#pragma once

#include <optional>

#include "object/delta_type_class.h"

#include "xdr/storage.h"

namespace scs
{

class StorageDelta;

class ProxyApplicator
{
	#if __cpp_lib_optional >= 202106L
		constexpr static std::optional<StorageObject> null_obj = std::nullopt;
	#else
		const std::optional<StorageObject> null_obj = std::nullopt;
	#endif
	

	std::optional<StorageObject> base;

	DeltaTypeClass typeclass;

	bool is_deleted = false;

	/* -- mem -- */

	bool mem_set_called = false;

	/* -- nn int64 -- */

	bool nn_int64_set_called = false;

public:

	ProxyApplicator(std::optional<StorageObject> const& base)
		: base(base)
		, typeclass()
		{}

	// no-op if result is false, apply if result is true
	bool 
	__attribute__((warn_unused_result))
	try_apply(StorageDelta const& delta);

	std::optional<StorageObject> const&
	get() const;

	const DeltaTypeClass& get_tc() const
	{
		return typeclass;
	}

};

} /* scs */
