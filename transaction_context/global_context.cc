#include "transaction_context/global_context.h"

#include "transaction_context/execution_context.h"

namespace scs
{


GlobalContext::GlobalContext()
	: contract_db()
	, state_db()
	, address_db()
	, engine(&ExecutionContext<TxContext>::static_syscall_handler)
	{}

GroundhogGlobalContext::GroundhogGlobalContext()
	: contract_db()
	, state_db()
	, address_db()
	, engine(&ExecutionContext<GroundhogTxContext>::static_syscall_handler)
	{}

SisyphusGlobalContext::SisyphusGlobalContext()
	: contract_db()
	, state_db()
	, address_db()
	, engine(&ExecutionContext<SisyphusTxContext>::static_syscall_handler)
	{}

} // namespace scs

