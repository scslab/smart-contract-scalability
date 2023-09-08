/**
 * Copyright 2023 Geoffrey Ramseyer
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "phase/phases.h"

#include "threadlocal/threadlocal_context.h"

#include "state_db/state_db.h"

#include "contract_db/contract_db.h"

#include "transaction_context/global_context.h"

#include "transaction_context/transaction_context.h"

#include "groundhog/types.h"

#include <utils/time.h>
#include <tbb/task_group.h>

namespace scs
{

void phase_finish_block(GlobalContext& global_structures, GroundhogBlockContext& block_structures)
{
	auto ts = utils::init_time_measurement();
	tbb::task_group g;
	g.run([&] () {
		block_structures.tx_set.finalize();
	});
	block_structures.modified_keys_list.merge_logs();
	std::printf("keylist merge %lf\n", utils::measure_time(ts));
	global_structures.contract_db.commit();
	std::printf("contract db commit %lf\n", utils::measure_time(ts));
	global_structures.state_db.commit_modifications(block_structures.modified_keys_list);
	std::printf("commit statedb %lf\n", utils::measure_time(ts));
	g.wait();
	std::printf("task group wait %lf\n", utils::measure_time(ts));
	ThreadlocalContextStore::post_block_clear<GroundhogTxContext>();
	std::printf("post block tlcs clear %lf\n", utils::measure_time(ts));
}

void phase_undo_block(GlobalContext& global_structures, GroundhogBlockContext& block_structures)
{
	block_structures.modified_keys_list.merge_logs();

	global_structures.contract_db.rewind();
	global_structures.state_db.rewind_modifications(block_structures.modified_keys_list);

	ThreadlocalContextStore::post_block_clear<GroundhogTxContext>();
}

} /* scs */
