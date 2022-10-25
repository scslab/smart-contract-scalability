#pragma once

#include <optional>

#include "xdr/storage.h"
#include "xdr/storage_delta.h"

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

	// type specific methods
	// returns nullopt if type mismatch
	std::optional<int64_t>
	get_base_nnint64_set_value() const;
};

} /* scs */
