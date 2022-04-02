#include "transaction_context/method_invocation.h"

#include <iomanip>
#include <sstream>

#include "mtt/utils/serialize_endian.h"

namespace scs
{

std::string 
MethodInvocation::get_invocable_methodname() const
{
	std::stringstream ss;
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
