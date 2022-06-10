#pragma once

#include "xdr/storage_delta.h"
#include "xdr/storage.h"

#include <optional>

namespace scs
{

struct ObjectModificationContext
{

	bool is_deleted_first = false;
	bool is_deleted_last = false;
//	std::optional<ObjectType> current_type;

	/* -- raw memory mod records -- */

	bool raw_mem_set_called = false;

	/* -- int64 records -- */

	bool int64_set_add_called = false;
	int64_t subtracted_amount = 0;
	uint64_t added_amount = 0;

	ObjectModificationContext(std::optional<StorageObject> const& obj);

	void
	accept_mod(StorageDelta const& delta);
};

} /* scs */
