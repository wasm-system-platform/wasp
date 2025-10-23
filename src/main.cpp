#include <fstream>
#include <iostream>

#include "devices/timer.hpp"
#include "fmt/base.h"
#include "grammar/module.hpp"
#include "hw/wasm_disk.hpp"
#include "runtime/context_manager.hpp"
#include "runtime/instance.hpp"

int main() {
    /*
    std::ifstream test_file;
    test_file.open("kernel.wasm", std::ios::in | std::ios::binary);

    Expected<grammar::Module> module = grammar::Module::parse(test_file);
    if (!module) {
        std::cout << module.error().toString() << std::endl;
        return -1;
    }

    runtime::ExternalFunction interrupt_enable =
        runtime::ExternalFunction::create([](runtime::Instance& instance) {
            instance.getGlobalState().enableInterrupts();
        });

    std::function<int32_t(runtime::Instance&)> interrupt_disable_func =
        [](runtime::Instance& instance) -> int32_t {
        return instance.getGlobalState().disableInterrupts();
    };

    runtime::ExternalFunction interrupt_disable =
        runtime::ExternalFunction::create(interrupt_disable_func);

    runtime::ExternalFunction console_write = runtime::ExternalFunction::create(
        [](runtime::Instance& instance, uint32_t offset, uint32_t len) {
            std::vector<uint8_t>& memory =
                instance.getGlobalState().getMemory();

            if (offset + len < memory.size()) {
                std::string_view msg(reinterpret_cast<char*>(&memory[offset]),
                                     len);
                fmt::println("console: {}", msg);
            }
        });

    runtime::ExternalFunction terminal_write =
        runtime::ExternalFunction::create([](runtime::Instance& instance,
                                             uint32_t offset, uint32_t len) {
            std::vector<uint8_t>& memory =
                instance.getGlobalState().getMemory();

            if (offset + len < memory.size()) {
                std::string_view msg(reinterpret_cast<char*>(&memory[offset]),
                                     len);
                fmt::println("terminal: {}", msg);
            }
        });

    runtime::ExternalFunction timer_set_interval =
        runtime::ExternalFunction::create(
            [](runtime::Instance& instance, uint32_t timout) -> void {
                instance.getTimer().setInterval(timout);
            });

    std::function<int32_t(runtime::Instance&)> context_get_func =
        [](runtime::Instance& instance) -> uint32_t {
        return instance.getActiveContext().getId();
    };

    runtime::ExternalFunction context_get =
        runtime::ExternalFunction::create(context_get_func);

    std::function<int32_t(runtime::Instance&, int32_t, int32_t)>
        context_create_func = [](runtime::Instance& instance,
                                 uint32_t entry_func_idx,
                                 int32_t param1) -> uint32_t {
        runtime::ContextManager& mgr = runtime::ContextManager::instance();
        size_t id = mgr.createContext();
        mgr.prepareContext(id, entry_func_idx, param1);
        return id;
    };

    runtime::ExternalFunction context_create =
        runtime::ExternalFunction::create(context_create_func);

    std::function<void(runtime::Instance&, int32_t)> context_switch_func =
        [](runtime::Instance& instance, uint32_t context_id) -> void {
        runtime::ContextManager& mgr = runtime::ContextManager::instance();
        instance.setContext(mgr.getContext(context_id));
        fmt::println("context_switch");
    };

    runtime::ExternalFunction context_switch =
        runtime::ExternalFunction::create(context_switch_func);

    Expected<WasmDisk> disk_exp = WasmDisk::create("rootfs.img");
    if (!disk_exp) {
        std::cout << disk_exp.error().toString() << std::endl;
        return -1;
    }

    std::function<int32_t(runtime::Instance&)> wasmdisk_size_func =
        [&disk = *disk_exp](runtime::Instance& instance) -> uint32_t {
        return disk.size();
    };
    runtime::ExternalFunction wasmdisk_size =
        runtime::ExternalFunction::create(wasmdisk_size_func);

    std::function<void(runtime::Instance&, int32_t, int32_t, int32_t)>
        wasmdisk_read_func = [&disk = *disk_exp](runtime::Instance& instance,
                                                 uint32_t dst, uint32_t offset,
                                                 uint32_t count) -> void {
        fmt::println("dst={}", dst);
        disk.read(&instance.getGlobalState().getMemory()[dst], offset, count);
    };
    runtime::ExternalFunction wasmdisk_read =
        runtime::ExternalFunction::create(wasmdisk_read_func);

    std::function<void(runtime::Instance&, int32_t, int32_t, int32_t)>
        wasmdisk_write_func = [&disk = *disk_exp](runtime::Instance& instance,
                                                  uint32_t src, uint32_t offset,
                                                  uint32_t count) -> void {
        disk.write(&instance.getGlobalState().getMemory()[src], offset, count);
    };
    runtime::ExternalFunction wasmdisk_write =
        runtime::ExternalFunction::create(wasmdisk_write_func);

    Expected<runtime::Instance> instance = runtime::Instance::create(
        *module, {
                     {"interrupt.enable", interrupt_enable},
                     {"interrupt.disable", interrupt_disable},
                     {"console.write", console_write},
                     {"terminal.write", terminal_write},
                     {"timer.set_interval", timer_set_interval},
                     {"context.get", context_get},
                     {"context.create", context_create},
                     {"context.switch", context_switch},
                     {"wasmdisk.size", wasmdisk_size},
                     {"wasmdisk.read", wasmdisk_read},
                     {"wasmdisk.write", wasmdisk_write},
                 });
    if (!instance) {
        std::cout << instance.error().toString() << std::endl;
        return -1;
    }*/

    auto kernel_result =
        runtime::Instance::createKernel("kernel.wasm", "rootfs.img");
    if (!kernel_result) {
        std::cout << kernel_result.error().toString() << std::endl;
        return -1;
    }

    runtime::Instance& kernel = **kernel_result;

    Expected<void> result = kernel.call("kernel_init");
    if (!result) {
        std::cout << result.error().toString() << std::endl;
        return -1;
    }

    // fmt::println("result: {}", *result);

    return 0;
}
