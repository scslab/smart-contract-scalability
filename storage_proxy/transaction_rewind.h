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

#include <vector>

#include "contract_db/contract_db_proxy.h"
#include "object/revertable_object.h"

namespace scs {

namespace detail {

template<typename... Committable>
class RewindVector;

template<typename T, typename... U>
class RewindVector<T, U...> : public RewindVector<U...>
{
    std::vector<T> objs;

  public:
    template<typename acc,
             std::enable_if<std::is_same<T, acc>::value>::type* = nullptr>
    void add(acc&& new_obj)
    {
        objs.emplace_back(std::move(new_obj));
    }

    template<typename acc,
             std::enable_if<!std::is_same<T, acc>::value>::type* = nullptr>
    void add(acc&& new_obj)
    {
        RewindVector<U...>::add(std::move(new_obj));
    }

    void commit()
    {
        for (auto& obj : objs) {
            obj.commit();
        }
        RewindVector<U...>::commit();
    }
};

template<>
class RewindVector<>
{
  public:
    void commit() {}
};

} // namespace detail

struct TransactionRewind
    : public detail::RewindVector<RevertableObject::DeltaRewind,
                                  ContractCreateClosure,
                                  ContractDeployClosure>
{};

} // namespace scs
