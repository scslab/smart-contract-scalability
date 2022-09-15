#pragma once

#include "object/revertable_object.h"
#include "contract_db/contract_db_proxy.h"

namespace scs
{

namespace detail
{

template<typename... Committable>
class RewindVector;

template<typename T, typename... U>
class RewindVector<T, U...>
    : public RewindVector<U...>
    {
    	std::vector<T> objs;
public:
    	template<typename acc,
             std::enable_if<std::is_same<T, acc>::value>::type* = nullptr>
        void add(acc&& new_obj)
        {
        	objs.emplace_back(std::move(new_obj));
        }

		template<typename acc,
            std::enable_if<!std::is_same<T, acc>::value>::type* = nullptr>
        void add(acc&& new_obj)
        {
        	RewindVector<U...>::add(std::move(new_obj));
        }

        void commit()
        {
        	for (auto& obj : objs)
        	{
        		obj.commit();
        	}
        	RewindVector<U...>::commit();
        }
    };

template<>
class RewindVector<>
{
public:
	void commit()
	{}
};

}

struct TransactionRewind : public detail::RewindVector<RevertableObject::DeltaRewind, ContractCreateClosure, ContractDeployClosure>
{};

}