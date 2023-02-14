#pragma once

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
