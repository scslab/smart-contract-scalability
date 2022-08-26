#pragma once

namespace scs
{

template<typename T>
class AtomicSingleton
{
	mutable std::atomic_flag initialized;
	mutable std::atomic<T*> value;

	T* conditional_init(auto&&... args) const
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
				return res;
			}
		}
	}

public:

	AtomicSingleton()
		: initialized()
		, value()
		{}

	T& get(auto&&... args)
	{
		return *conditional_init(args...);
	}

	const T& get(auto&&... args) const
	{
		return *conditional_init(args...);
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
