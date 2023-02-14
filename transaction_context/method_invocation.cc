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

#include "transaction_context/method_invocation.h"

#include <iomanip>
#include <sstream>

#include <utils/serialize_endian.h>

namespace scs
{

MethodInvocation::MethodInvocation(TransactionInvocation const& root_invocation)
	: addr(root_invocation.invokedAddress)
	, method_name(root_invocation.method_name)
	, calldata(root_invocation.calldata)
	{}

MethodInvocation::MethodInvocation(const Address& addr, uint32_t method, std::vector<uint8_t>&& calldata)
	: addr(addr)
	, method_name(method)
	, calldata(std::move(calldata))
	{}


std::string 
MethodInvocation::get_invocable_methodname() const
{
	std::stringstream ss;
	ss << "pub";
	ss << std::hex << std::uppercase << std::setw(2) << std::setfill('0');
	uint8_t buf[4];

	utils::write_unsigned_little_endian(buf, method_name);

	for (auto i = 0u; i < 4; i++)
	{
		ss << std::setfill('0') << std::setw(2) << static_cast<int>(buf[i]);
	}

	return ss.str();
}

} /* scs */
