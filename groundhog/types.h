
#pragma once

#include "transaction_context/transaction_context.h"
#include "state_db/state_db.h"

namespace scs
{

typedef TransactionContext<StateDB> GroundhogTxContext;

}

