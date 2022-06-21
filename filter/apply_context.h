#pragma once

#include <memory>

#include "xdr/storage.h"

#include <optional>

namespace scs
{

class DeltaTypeClass;
class DeltaVector;
class TxBlock;

class ApplyContext
{

protected:

	DeltaTypeClass const& dtc;
	ApplyContext(DeltaTypeClass const& dtc);

	virtual 
	std::optional<StorageObject> 
	get_object_() const = 0;

public:

	virtual void add_vec(DeltaVector const& v, const TxBlock& txs) = 0;
	
	std::optional<StorageObject>
	get_object() const;

};

std::unique_ptr<ApplyContext> 
make_apply_context(
	DeltaTypeClass const& dtc);

} /* scs */
