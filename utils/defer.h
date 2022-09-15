#pragma once

namespace scs {

template<typename lambda_t>
struct defer
{
    lambda_t obj;

    ~defer() { obj(); }
};

} // namespace scs