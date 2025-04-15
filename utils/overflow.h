#pragma once

#include <concepts>
#include<type_traits>

namespace scs {

template<std::integral T, std::integral U, std::integral Result>
inline bool
integer_add_overflow(T a, U b, Result *out)
{
  return __builtin_add_overflow(a, b, out);
}

template<typename R = void, std::integral T, std::integral U>
inline bool
integer_add_overflow(T a, U b)
{
  using Result = std::conditional_t<std::is_void_v<R>, decltype(a + b), R>;
  // clang doensn't have add_overflow_p but does have add_overflow
#if defined(__builtin_add_overflow_p)
  return __builtin_add_overflow_p(a, b, Result{0});
#else
  Result r;
  return integer_add_overflow(a, b, &r);
#endif
}

}
