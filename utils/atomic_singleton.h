#pragma once

namespace scs
{

template<typename T>
class AtomicSingleton
{
	mutable std::atomic_flag initialized;
	mutable std::atomic<T*> value;

public:

	AtomicSingleton()
		: initialized()
		, value()
		{}

	T& get(auto&&... args)
	{
		if (!initialized.test_and_set(std::memory_order_relaxed))
		{
			value.store(new T(args...), std::memory_order_relaxed);
		} 

		while(true)
		{
			auto* res = value.load(std::memory_order_relaxed);
			if (res != nullptr)
			{
				return *res;
			}
		}
	}

	// obv cannot access object concurrent with dtor, must not double destruct
	~AtomicSingleton()
	{
		if (value.load() != nullptr)
		{
			delete (value.load());
		}
	}
};

} /* scs */
