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

};
