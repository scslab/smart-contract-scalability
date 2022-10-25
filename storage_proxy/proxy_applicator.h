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

	std::optional<StorageObject> current;

	std::optional<StorageDelta> overall_delta;
	bool is_deleted = false;

	bool delta_apply_type_guard(StorageDelta const& d) const;

public:

	ProxyApplicator(std::optional<StorageObject> const& base)
		: current(base)
		, overall_delta(std::nullopt)
		{}

	// no-op if result is false, apply if result is true
	bool 
	__attribute__((warn_unused_result))
	try_apply(StorageDelta const& delta);

	std::optional<StorageObject> const&
	get() const;

	std::vector<StorageDelta> get_deltas() const;
};

} /* scs */
