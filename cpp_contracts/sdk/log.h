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
#include "sdk/alloc.h"
#include "sdk/concepts.h"

#include <string.h>

#define LOG_MAX_STR_LEN 256

namespace sdk
{

namespace detail
{

BUILTIN("log")
void log(uint32_t offset, uint32_t len);

BUILTIN("print_debug")
void print_debug(uint32_t offset, uint32_t len);

BUILTIN("print_c_str")
void print_c_str(uint32_t offset, uint32_t len);

} /* detail */

template<TriviallyCopyable T>
void log(T const& val)
{
	detail::log(sdk::to_offset(&val), sizeof(T));
}

template<TriviallyCopyable T>
void print_debug(T const& val)
{
	detail::print_debug(sdk::to_offset(&val), sizeof(T));
}

void print(const char* str, uint32_t len)
{
	detail::print_c_str(sdk::to_offset(str), len);
}

void print(const char* str)
{
	print(str, strnlen(str, LOG_MAX_STR_LEN));
}

} /* sdk */
