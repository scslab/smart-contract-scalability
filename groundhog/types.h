
#pragma once

#include "transaction_context/transaction_context.h"
#include "state_db/state_db_v2.h"

namespace scs
{

typedef TransactionContext<StateDBv2> GroundhogTxContext;

}

