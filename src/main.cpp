#include <iostream>

#include <cxxopts.hpp>

#include "runtime/kernel.hpp"

int main(int argc, char** argv) {

    cxxopts::Options options("wasm-sp");

    options.add_options()("k,kernel", "Path to kernel WASM file",
                          cxxopts::value<std::string>())(
        "r,rootfs", "Path to rootfs image",
        cxxopts::value<std::string>())("h,help", "Show help")
        ("m,memory", "Kernel memory size in pages", cxxopts::value<int16_t>());

    auto args = options.parse(argc, argv);

    options.custom_help("-k <kernel.wasm> -r <rootfs.img>");

    if (args.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    if (args.count("kernel") == 0) {
        std::cout << "Option 'kernel' is required but not present\n"
                  << options.help() << std::endl;
        exit(1);
    }

    if (args.count("rootfs") == 0) {
        std::cout << "Option 'rootfs' is required but not present\n"
                  << options.help() << std::endl;
        exit(1);
    }

    auto kernel_exp = runtime::Kernel::create(args["kernel"].as<std::string>(),
                                              args["rootfs"].as<std::string>());
    if (!kernel_exp) {
        std::cout << kernel_exp.error().toString() << std::endl;
        return -1;
    }

    runtime::Instance& kernel = **kernel_exp;

    if (args.count("memory") != 0) {
        kernel.getGlobalState().getMemory().updateLimit(args["memory"].as<int16_t>());
    }

    Expected<void> result_exp = kernel.call("kernel_start", 0);
    if (!result_exp) {
        std::cout << result_exp.error().toString() << std::endl;
        return -1;
    }

    return 0;
}
