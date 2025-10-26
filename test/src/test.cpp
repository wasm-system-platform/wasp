#include <gtest/gtest.h>

#include "runtime/instance.hpp"
#include "util/error_handling.hpp"

static int ack(int m, int n) {
    if (m == 0) return n + 1;
    if (n == 0) return ack(m - 1, 1);
    return ack(m - 1, ack(m, n - 1));
}

TEST(Programs, Ackermann) {
    std::ifstream test_file;
    test_file.open("build/test/ackermann.wasm", std::ios::in | std::ios::binary);

    Expected<grammar::Module> module = grammar::Module::parse(test_file);
    if (!module) {
        std::cout << module.error().toString() << std::endl;
        FAIL();
    }

    auto instance = runtime::Instance::create(*module, {});
    if (!instance) {
        std::cout << instance.error().toString() << std::endl;
        FAIL();
    }

    auto result = (*instance)->callRet("main", 0, 0);

    EXPECT_EQ(ack(2, 3), *result);
}
