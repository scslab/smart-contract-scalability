#include "transaction_context/global_context.h"

namespace scs
{

GlobalContext::GlobalContext()
	: contract_db()
	, tx_block()
	, state_db()
	{}

/*
void 
StaticGlobalContext::set(GlobalContext* c)
{
	if (context != nullptr)
	{
		delete context;
	}
	context = c;
}

void
_free_global_context()
{
	auto* p = StaticGlobalContext::context;
	if (p != nullptr)
	{
		delete p;
	}
} */

} /* scs */
