#pragma once

#include <optional>

#include "object/object_modification_context.h"

#include "xdr/storage.h"

namespace scs
{

class StorageDelta;

class DeltaApplicator
{
	#if __cpp_lib_optional >= 202106L
		constexpr static std::optional<StorageObject> null_obj = std::nullopt;
	#else
		const std::optional<StorageObject> null_obj = std::nullopt;
	#endif

	std::optional<StorageObject> base;

	ObjectModificationContext mod_context;

public:

	DeltaApplicator(std::optional<StorageObject> const& base)
		: base(base)
		, mod_context(base)
		{}

	// no-op if result is false, apply if result is true
	bool 
	__attribute__((warn_unused_result))
	try_apply(StorageDelta const& delta);

	std::optional<StorageObject> const& 
	get() const;

};

} /* scs */
