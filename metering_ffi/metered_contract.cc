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

#include "metering_ffi/metered_contract.h"

namespace scs {

namespace detail {

/**
 * Imported from the rust library under ../metering
 */
extern "C"
{

// does NOT take ownership of data
metered_contract add_metering_ext(uint8_t const* data, uint32_t len);

void free_metered_contract(metered_contract contract);

}

metered_contract timed_add_metering_ext(uint8_t const* data, uint32_t len)
{
    auto out = add_metering_ext(data, len);
    return out;
}

} // namespace detail

MeteredContract::MeteredContract(std::shared_ptr<const Contract> unmetered)
    : base(detail::timed_add_metering_ext(unmetered->data(), unmetered->size()))
{}

MeteredContract::~MeteredContract()
{
    if (base.data != nullptr) {
        detail::free_metered_contract(base);
    }
}

} // namespace scs
