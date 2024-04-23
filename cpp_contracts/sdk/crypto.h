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

#include "sdk/macros.h"
#include "sdk/concepts.h"
#include "sdk/alloc.h"
#include "sdk/types.h"
#include "sdk/concepts.h"
#include "sdk/syscall.h"

#include <cstdint>
#include <optional>

namespace sdk
{

Hash hash(uint8_t const* data, uint32_t size)
{
	Hash out;
	detail::builtin_syscall(SYSCALLS::HASH,
		to_offset(data), size, to_offset(out.data()), 0, 0, 0);
	return out;
}

template <VectorLike T>
Hash hash(T const& object)
{
	return hash(object.data(), object.size());
}

template <VectorLike T>
void hash(T const& object, StorageKey& out)
{
	detail::builtin_syscall(SYSCALLS::HASH,
		to_offset(object.data()), object.size(), to_offset(out.data()),
		0, 0, 0);
}

template<TriviallyCopyable T>
Hash hash(T const& obj)
{
	return hash(reinterpret_cast<const uint8_t*>(&obj), sizeof(T));
}

template<TriviallyCopyable T>
void hash(T const& obj, StorageKey& out)
{
	detail::builtin_syscall(SYSCALLS::HASH,
		to_offset(&obj), sizeof(T), to_offset(out.data()),
		0, 0, 0);
}

template<TriviallyCopyable msg>
bool
check_sig_ed25519(
	const PublicKey& pk,
	const Signature& sig,
	const msg& m)
{
	return detail::builtin_syscall(SYSCALLS::VERIFY_ED25519,
	to_offset(pk.data()),
	to_offset(sig.data()),
	to_offset(&m),
	sizeof(msg),
	0, 0) != 0;
}

} /* sdk */
