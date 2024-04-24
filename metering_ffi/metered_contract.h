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

#include <cstdint>
#include <memory>

#include <utils/non_movable.h>

#include "xdr/types.h"

#include "contract_db/runnable_script.h"

namespace scs {

namespace detail {

// matches metering/inject_metering/src/lib.rs:metered_contract
struct metered_contract
{
    uint8_t* data;
    uint32_t len;
    uint32_t capacity;
};

} // namespace detail

class MeteredContract : public utils::NonMovableOrCopyable
{
    detail::metered_contract base;

  public:
    MeteredContract(std::shared_ptr<const Contract> unmetered);

    ~MeteredContract();

    operator bool() const { return base.data != nullptr; }

    RunnableScriptView to_view() const;
    Hash hash() const;
};

using metered_contract_ptr_t = std::shared_ptr<const MeteredContract>;

} // namespace scs
