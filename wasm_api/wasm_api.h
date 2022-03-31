#pragma once

#include "contract_db/contract_db.h"

#include "transaction_context/method_invocation.h"

#include "wasm_api/wasm3.h"

#include "xdr/transaction.h"
#include "xdr/types.h"

namespace scs {

class WasmRuntime;
class Wasm3_WasmRuntime;

class WasmError : public std::runtime_error {
public:
    explicit WasmError(std::string const& err) : std::runtime_error(err) {}
};

class WasmContext {

protected:
	ContractDB& contract_db;

public:

	WasmContext(ContractDB& db)
		: contract_db(db)
		{}

	virtual 
	std::unique_ptr<WasmRuntime>
	new_runtime_instance(Address const& contract_address) = 0;

};

class Wasm3_WasmContext : public WasmContext {

	wasm3::environment env;

public:

	Wasm3_WasmContext(ContractDB& db)
		: WasmContext(db)
		{}

	std::unique_ptr<WasmRuntime> new_runtime_instance(Address const& contract_address) override final;

};

class WasmRuntime {

	WasmContext& originating_context;

protected:

	virtual std::pair<uint8_t*, uint32_t> get_memory() = 0;

	WasmRuntime(WasmContext& ctx)
		: originating_context(ctx)
		{}

public:

	virtual TransactionStatus invoke(MethodInvocation const& invocation) = 0;

	virtual void link_fn(
		const char* module_name, const char* fn_name, 
		void (*f) (int32_t, int32_t)) = 0;
	virtual void link_fn(
		const char* module_name, const char* fn_name, 
		void (*f) (int32_t, int32_t, int32_t, int32_t, int32_t)) = 0;
	virtual void link_fn(
		const char* module_name, const char* fn_name, 
		void (*f) (int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t)) = 0;

	template<typename ArrayLike>
	ArrayLike load_from_memory(int32_t offset, int32_t len)
	{
		auto [mem, mlen] = get_memory();
		if (mlen < offset + len) {
			throw WasmError("OOB Mem Access");
		}

		if (offset < 0 || len < 0) {
			throw WasmError("Invalid Mem Parameters");
		}

		return ArrayLike(mem + offset, mem + offset + len);
	}

	template <typename ArrayLike>
	ArrayLike load_from_memory_to_const_size_buf(int32_t offset)
	{
		ArrayLike out;
		const size_t len = out.size();

		auto [mem, mlen] = get_memory();
		if (mlen < offset + len) {
			throw WasmError("OOB Mem Access");
		}
		if (offset < 0 || len < 0) {
			throw WasmError("Invalid Mem Parameters");
		}

		memcpy(out.data(), mem, len);
		return out;
	}

	template<typename ArrayLike>
	void write_to_memory(ArrayLike const& array, uint32_t offset, uint32_t max_len)
	{

		if (array.size() > max_len)
		{
			throw WasmError("array is too big to write");
		}

		if (offset < 0 || max_len < 0) {
			throw WasmError("Invalid Mem Parameters");
		}

		auto [mem, mlen] = get_memory();

		if (mlen > offset + array.size()) {
			throw WasmError("OOB Mem Write");
		}

		memcpy(mem + offset, array.data(), array.size());
	}
};

class Wasm3_WasmRuntime : public WasmRuntime {

	wasm3::runtime runtime;
	wasm3::module module;

	std::pair<uint8_t*, uint32_t> get_memory() override final
	{
		return runtime.get_memory();
	}

public:

	Wasm3_WasmRuntime(wasm3::runtime&& r, wasm3::module&& m, WasmContext& ctx)
		: WasmRuntime(ctx)
		, runtime(std::move(r))
		, module(std::move(m))
		{}

	void link_fn(const char* module_name, const char* fn_name, void(*f)(int32_t, int32_t)) override final
	{
		module.link_optional(module_name, fn_name, f);
	}
	void link_fn(
		const char* module_name, 
		const char* fn_name, 
		void (*f)(int32_t, int32_t, int32_t, int32_t, int32_t)) override final
	{
		module.link_optional(module_name, fn_name, f);
	}
	void link_fn(
		const char* module_name, 
		const char* fn_name, 
		void (*f) (int32_t, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t)) override final
	{
		module.link_optional(module_name, fn_name, f);
	}

	TransactionStatus 
	invoke(MethodInvocation const& invocation) override final;
};


} /* namespace scs */
