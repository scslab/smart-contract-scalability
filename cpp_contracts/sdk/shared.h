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

namespace scs
{

namespace detail
{

constexpr static 
void write_uint64_t(uint8_t* out, uint64_t value)
{
	for (size_t i = 0; i < 8; i++)
	{
		out[i] = (value >> (8*i)) & 0xFF;
	}
}

}

template<typename OutputArray>
constexpr static 
OutputArray
make_static_32bytes(uint64_t a, uint64_t b = 0, uint64_t c = 0, uint64_t d = 0)
{
	OutputArray out = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	uint8_t* data = out.data();
	detail::write_uint64_t(data, a);
	detail::write_uint64_t(data + 8, b);
	detail::write_uint64_t(data + 16, c);
	detail::write_uint64_t(data + 24, d);
	return out;
}

}
