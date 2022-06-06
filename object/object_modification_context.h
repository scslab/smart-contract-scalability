#pragma once

#include "xdr/storage_delta.h"
#include "xdr/storage.h"

#include <optional>

namespace scs
{

struct ObjectModificationContext
{

	bool is_deleted = false;
	std::optional<ObjectType> current_type;

	/* -- raw memory mod records -- */

	bool raw_mem_set_called = false;

	/* -- int records -- */

	bool int_set_called = false;
	bool int_add_sub_called = false;
	bool int_max_called = false;
	bool int_min_called = false;
	bool int_xor_called = false;
	ObjectModificationContext(std::optional<StorageObject> const& obj);


	bool
	__attribute__((warn_unused_result))
	can_accept_mod(DeltaType dt) const;

	void
	accept_mod(DeltaType dt);
};

} /* scs */
