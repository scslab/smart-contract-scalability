#include "object/delta_type_class.h"

#include "xdr/storage_delta.h"

#include <atomic>

namespace scs
{

class DeltaTypeClassFilter
{

protected:
	const DeltaTypeClass tc;

public:

	DeltaTypeClassFilter(const DeltaTypeClass& tc)
		: tc(tc)
		{}

	bool 
	matches_typeclass(const StorageDelta& d) const
	{
		return tc.accepts(d);
	}

	/**
	 * Important invariant:
	 * Once this returns false,
	 * no subsequent call depends
	 * on any previous call.
	 * That is to say,
	 * the results would not change
	 * if we had inserted some additional
	 * call to add() previously.
	 * 
	 * E.g. NNINT64: set_add stops allowing
	 * subtractions once the total subtracted
	 * exceeds the set amount.
	 * Adding more subtractions earlier
	 * does not allow a subsequent subtraction to proceed.
	 * 
	 * This is why, when a (e.g. large) sub call would
	 * overflow the set bound, we still update total_subtracted.
	 * (This helps enable a parallelizable implementation).
	 */
	virtual bool 
	add(StorageDelta const& d) = 0;

	static std::unique_ptr<DeltaTypeClassFilter> 
	make(DeltaTypeClass const& base);
};

class DeltaTypeClassApplier
{

protected:
	const DeltaTypeClass tc;
	std::atomic_flag is_deleted;

public:

	DeltaTypeClassApplier(const DeltaTypeClass& tc)
		: tc(tc)
		, is_deleted(false)
		{}

	bool 
	matches_typeclass(const StorageDelta& d) const
	{
		return tc.accepts(d);
	}

	virtual void 
	add(StorageDelta const& d) = 0;

	virtual
	//first: should we delete object?
	//second: replacement object, if we can build one
	// second is only opt if first == false
	// if first is false and second is nullopt,
	// then current value stands without change
	std::pair<bool, std::optional<StorageObject>>
	get_result() const = 0;

	static std::unique_ptr<DeltaTypeClassApplier> 
	make(DeltaTypeClass const& base);
};


} /* scs */
