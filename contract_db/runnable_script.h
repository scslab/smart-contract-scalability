<<<<<<< HEAD
#pragma once

#include <cstdint>

namespace scs {

	struct RunnableScriptView
	{
		const uint8_t* data;
	        uint32_t len;

	    operator bool() { return data != nullptr; }
	};

	constexpr static RunnableScriptView null_script{ .data = nullptr, .len = 0 };

} // namespace scs

