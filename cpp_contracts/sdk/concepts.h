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

#pragma once

#include <cstdint>
#include <type_traits>

namespace sdk
{

template<typename T>
concept TriviallyCopyable
= requires {
	typename std::enable_if<std::is_trivially_copyable<T>::value>::type;
};

template<TriviallyCopyable T>
uint32_t 
to_offset(const T* addr)
{
	static_assert(sizeof(void*) == 4, "wasm pointer size");
	return reinterpret_cast<uint32_t>(reinterpret_cast<const void*>(addr));
}

template<typename T>
concept VectorLike
= requires (T object)
{
	object.data();
	object.size();
} && !TriviallyCopyable<T>;

} /* sdk */
