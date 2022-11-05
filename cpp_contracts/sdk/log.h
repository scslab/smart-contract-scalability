#pragma once

#include "sdk/macros.h"
#include "sdk/alloc.h"
#include "sdk/concepts.h"

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

} /* sdk */
