#include <gtest/gtest.h>

#include "runtime/instance.hpp"
#include "util/error_handling.hpp"

static int ack(int m, int n) {
    if (m == 0)
        return n + 1;
    if (n == 0)
        return ack(m - 1, 1);
    return ack(m - 1, ack(m, n - 1));
}

TEST(Programs, Ackermann) {
    std::ifstream test_file;
    test_file.open("test/ackermann.wasm", std::ios::in | std::ios::binary);

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

    auto t0_native = std::chrono::steady_clock::now();
    int32_t native = ack(3, 11);
    auto t1_native = std::chrono::steady_clock::now();
    auto native_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         t1_native - t0_native)
                         .count();

    auto t0_interp = std::chrono::steady_clock::now();
    auto result = (*instance)->callRet("main", 0, 0);
    auto t1_interp = std::chrono::steady_clock::now();
    auto interp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                         t1_interp - t0_interp)
                         .count();

    double factor =
        static_cast<double>(interp_ns) / static_cast<double>(native_ns);
    double slower_pct = (factor - 1.0) * 100.0;

    std::cout << "[perf] native: " << native_ns / 1'000'000.0
              << " ms, interpreter: " << interp_ns / 1'000'000.0 << " ms";
    std::cout << " (+" << slower_pct << "%)" << std::endl;

    EXPECT_EQ(native, *result);
}
