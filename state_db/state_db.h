#pragma once

namespace scs
{

class StateDB
{

	std::map<AddressAndKey, StorageObject> state_db;

public:


	void filter_invalid_deltas(SerialDeltaBatch& delta_batch);


	// TODO stuff

};


} /* scs */
