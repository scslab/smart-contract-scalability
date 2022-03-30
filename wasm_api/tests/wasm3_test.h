#include <cxxtest/TestSuite.h>

#include "wasm_api/wasm3.h"

#include "test_utils/load_wasm.h"

using namespace scs;
using namespace scs::test;

class Wasm3TestSuite : public CxxTest::TestSuite {

public:

	void test_call_simple()
	{
		wasm3::environment e;
		
		Contract c = load_wasm_from_file("wasm_api/tests/wat/test_call_simple.wasm");

		auto m = e.parse_module(c.data(), c.size());

		auto r = e.new_runtime(65536);

		r.load(m);

		auto f = r.find_function("add");

		int32_t res = f.template call<int32_t>(15, 20);
		TS_ASSERT_EQUALS(res, 35);
	}

	void test_set_memory()
	{
		wasm3::environment e;
		auto c = load_wasm_from_file("wasm_api/tests/wat/test_set_memory.wasm");

		auto m = e.parse_module(c.data(), c.size());

		auto r = e.new_runtime(65536);

		r.load(m);

		auto sz = r.find_function("size");

		TS_ASSERT_EQUALS(sz.template call<int32_t>(), 1);

		auto store = r.find_function("store");

		store.call(0, 0x12345678);

		store.call(4, 0xABCDEF90);

		auto load = r.find_function("load");

		uint32_t mem0 = load.template call<int32_t>(0);

		TS_ASSERT_EQUALS(mem0, 0x12345678);

		auto load8 = r.find_function("load8");

		TS_ASSERT_EQUALS(load8.template call<int32_t>(0), 0x78);
		TS_ASSERT_EQUALS(load8.template call<int32_t>(1), 0x56);

		auto load16 = r.find_function("load16");

		TS_ASSERT_EQUALS(load16.template call<int32_t>(3), 0x9012);
	}

};
