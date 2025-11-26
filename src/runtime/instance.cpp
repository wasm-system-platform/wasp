#include <fmt/format.h>
#include <sstream>

#include "devices/keyboard.hpp"
#include "hw/wasm_disk.hpp"
#include "runtime/context_manager.hpp"
#include "runtime/instance.hpp"
#include "runtime/process_manager.hpp"

namespace runtime {

Expected<std::shared_ptr<Instance>>
Instance::create(const grammar::Module& module,
                 const std::unordered_map<std::string, Function>& imports) {
    Expected<GlobalState> store_exp = GlobalState::create(module, imports);
    if (!store_exp)
        return Unexpected(PROPAGATE(store_exp));

    grammar::ImportSection import_section = module.getImportSection();
    grammar::FunctionSection function_section = module.getFunctionSection();
    grammar::TypeSection type_section = module.getTypeSection();
    grammar::ExportSection export_section = module.getExportSection();

    std::vector<uint32_t> type_indices = function_section.getTypes();
    std::hash<FunctionType> signature_hasher;

    std::unordered_map<std::string, Export> exports;
    for (const auto& func : export_section.getFunctionExports()) {
        uint32_t type_index =
            type_indices[func.getIdx() -
                         import_section.getFunctionImports().size()];

        Expected<FunctionType> func_type_exp =
            type_section.getFunctionType(type_index);
        if (!func_type_exp)
            return Unexpected(PROPAGATE(func_type_exp));
        const FunctionType& func_type = *func_type_exp;

        size_t signature = signature_hasher(func_type);
        exports.emplace(func.getName(),
                        Export{func.getIdx(), func_type, signature});
    }

    return std::make_shared<Instance>(
        Instance(std::move(*store_exp), std::move(exports), module));
}

Expected<std::shared_ptr<Instance>>
Instance::createKernel(const std::string& kernel_path,
                       const std::string& rootfs_path) {
    std::ifstream kernel_fstream;
    kernel_fstream.open("kernel.wasm", std::ios::in | std::ios::binary);

    Expected<grammar::Module> module = grammar::Module::parse(kernel_fstream);
    if (!module)
        return Unexpected(PROPAGATE(module));

    // Interrupts
    std::function<void(Instance & instance)> interrupt_enable_func =
        [](Instance& instance) -> void {
        instance.getGlobalState().enableInterrupts();
    };
    std::function<int32_t(Instance&)> interrupt_disable_func =
        [](Instance& instance) -> int32_t {
        return instance.getGlobalState().disableInterrupts();
    };

    Function interrupt_enable = Function::createExternal(interrupt_enable_func);
    Function interrupt_disable =
        Function::createExternal(interrupt_disable_func);

    // Terminal
    std::function<void(Instance&, int32_t, int32_t)> terminal_write_func =
        [](Instance& instance, uint32_t offset, uint32_t len) -> void {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();

        if (offset + len < memory.size()) {
            std::string_view msg(reinterpret_cast<char*>(&memory[offset]), len);
            fmt::print("terminal: {}", msg);
        }
    };

    Function terminal_write = Function::createExternal(terminal_write_func);

    // Console
    std::function<void(Instance&, int32_t, int32_t)> console_write_func =
        [](Instance& instance, uint32_t offset, uint32_t len) -> void {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();

        if (offset + len < memory.size()) {
            std::string_view msg(reinterpret_cast<char*>(&memory[offset]), len);
            fmt::print("{}", msg);
        }
    };

    Function console_write = Function::createExternal(console_write_func);

    // Context
    std::function<int32_t(Instance&)> context_get_func =
        [](Instance& instance) -> uint32_t {
        return instance.getActiveContext().getId();
    };
    std::function<int32_t(Instance&, int32_t, int32_t)> context_create_func =
        [](Instance& instance, uint32_t entry_func_idx,
           int32_t param1) -> uint32_t {
        ContextManager& mgr = ContextManager::instance();
        size_t id = mgr.createContext();
        mgr.prepareContext(id, entry_func_idx, param1);
        return id;
    };
    std::function<void(Instance&, int32_t)> context_switch_func =
        [](Instance& instance, uint32_t context_id) -> void {
        ContextManager& mgr = ContextManager::instance();
        instance.setContext(mgr.getContext(context_id));
    };

    Function context_get = Function::createExternal(context_get_func);
    Function context_create = Function::createExternal(context_create_func);
    Function context_switch = Function::createExternal(context_switch_func);

    // WasmDisk
    Expected<WasmDisk> disk_exp = WasmDisk::create("rootfs.img");
    if (!disk_exp)
        return Unexpected(PROPAGATE(disk_exp));

    std::shared_ptr<WasmDisk> disk =
        std::make_shared<WasmDisk>(std::move(*disk_exp));

    std::function<int32_t(Instance&)> wasmdisk_size_func =
        [disk](Instance& instance) -> uint32_t { return disk->size(); };
    std::function<void(Instance&, int32_t, int32_t, int32_t)>
        wasmdisk_read_func = [disk](Instance& instance, uint32_t dst,
                                    uint32_t offset, uint32_t count) -> void {
        disk->read(&instance.getGlobalState().getMemory()[dst], offset, count);
    };
    std::function<void(Instance&, int32_t, int32_t, int32_t)>
        wasmdisk_write_func = [disk](Instance& instance, uint32_t src,
                                     uint32_t offset, uint32_t count) -> void {
        disk->write(&instance.getGlobalState().getMemory()[src], offset, count);
    };

    Function wasmdisk_size = Function::createExternal(wasmdisk_size_func);
    Function wasmdisk_read = Function::createExternal(wasmdisk_read_func);
    Function wasmdisk_write = Function::createExternal(wasmdisk_write_func);

    // Process
    std::function<int32_t(Instance&, int32_t, int32_t)> process_load_func =
        [](Instance& instance, uint32_t program_offset,
           uint32_t size) -> int32_t {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
        if (program_offset + size > memory.size()) {
            return -1;
        }
        std::string program_bytes(
            reinterpret_cast<char*>(&memory[program_offset]), size);
        auto program_result = createUserProgram(program_bytes, instance);
        if (!program_result) {
            fmt::println("create user program: {}",
                         program_result.error().toString());
            return -1;
        }

        Instance& program = **program_result;
        Export start = program.exports_.find("_start")->second;

        ProcessManager& proc_mgr = ProcessManager::instance();
        uint32_t pid = proc_mgr.createProcess();
        proc_mgr.loadProgram(pid, instance, program, start.func_idx);
        return pid;
    };
    std::function<void(Instance&, int32_t)> process_unload_func =
        [](Instance& instance, uint32_t pid) -> void {};
    std::function<int32_t(Instance&, int32_t)> process_run_func =
        [](Instance& instance, uint32_t pid) -> int32_t {
        ProcessManager& proc_mgr = ProcessManager::instance();
        proc_mgr.runProcess(pid, instance);
        return 0;
    };
    std::function<void(Instance&, int32_t)> process_terminate_func =
        [](Instance& instance, uint32_t pid) -> void {
        ProcessManager& proc_mgr = ProcessManager::instance();
        Instance& proc = proc_mgr.getProcess(pid);
        proc.getActiveContext().setRunState(Context::RunState::rdy);
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)>
        process_read_func = [](Instance& instance, int pid, uint32_t dst,
                               uint32_t proc_src, uint32_t count) -> int32_t {
        std::vector<uint8_t>& mem = instance.getGlobalState().getMemory();
        ProcessManager& proc_mgr = ProcessManager::instance();
        std::vector<uint8_t>& proc_mem =
            proc_mgr.getProcess(pid).getGlobalState().getMemory();
        std::memcpy(&mem[dst], &proc_mem[proc_src], count);
        return count;
    };

    Function process_load = Function::createExternal(process_load_func);
    Function process_unload = Function::createExternal(process_unload_func);
    Function process_run = Function::createExternal(process_run_func);
    Function process_terminate =
        Function::createExternal(process_terminate_func);
    Function process_read = Function::createExternal(process_read_func);

    // Devices
    std::function<void(Instance&, int32_t)> timer_set_interval_func =
        [](Instance& instance, uint32_t timout) -> void {
        instance.getTimer().setInterval(timout);
    };

    Function timer_set_interval =
        Function::createExternal(timer_set_interval_func);

    Expected<std::shared_ptr<Instance>> instance_exp = Instance::create(
        *module, {
                     {"interrupt.enable", interrupt_enable},
                     {"interrupt.disable", interrupt_disable},
                     {"console.write", console_write},
                     {"console.debug", terminal_write},
                     {"context.get", context_get},
                     {"context.create", context_create},
                     {"context.switch", context_switch},
                     {"wasmdisk.size", wasmdisk_size},
                     {"wasmdisk.read", wasmdisk_read},
                     {"wasmdisk.write", wasmdisk_write},
                     {"process.load", process_load},
                     {"process.unload", process_unload},
                     {"process.run", process_run},
                     {"process.terminate", process_terminate},
                     {"process.read", process_read},
                     {"timer.set_interval", timer_set_interval},
                 });
    if (!instance_exp)
        return Unexpected(PROPAGATE(instance_exp));

    Instance& instance = **instance_exp;

    auto it = instance.exports_.find("interrupt_handler");
    if (it == instance.exports_.end())
        return Unexpected(ERROR("no interrupt handler found"));

    const Export& interrupt_handler = it->second;

    size_t interrupt_handler_sig =
        std::hash<FunctionType>()(FunctionType::ConsumerI32());
    if (interrupt_handler.signature != interrupt_handler_sig)
        return Unexpected(
            ERROR(fmt::format("interrupt handler has a wrong signature; "
                              "expected signature '{}' but found '{}'",
                              FunctionType::ConsumerI32().toString(),
                              interrupt_handler.func_type.toString())));

    instance.devices_.emplace(
        TIMER_PORT, std::make_shared<Timer>(interrupt_handler.func_idx));
    // instance.devices_.emplace(
    //     KEYBOARD_PORT,
    //     std::make_shared<Keyboard>(interrupt_handler.func_idx));

    it = instance.exports_.find("syscall_handler");
    if (it == instance.exports_.end())
        return Unexpected(ERROR("no system call handler found"));

    const Export& syscall_handler = it->second;

    size_t syscall_handler_sig = std::hash<FunctionType>()(
        FunctionType::ProducerI32_I32_I32_I32_I32_I32());
    if (syscall_handler.signature != syscall_handler_sig)
        return Unexpected(ERROR(fmt::format(
            "interrupt handler has a wrong signature; "
            "expected signature '{}' but found '{}'",
            FunctionType::ProducerI32_I32_I32_I32_I32_I32().toString(),
            syscall_handler.func_type.toString())));

    return instance_exp;
}

Expected<std::shared_ptr<Instance>>
Instance::createUserProgram(const std::string& program, Instance& parent) {
    std::stringstream program_stream(program);

    Expected<grammar::Module> module_exp =
        grammar::Module::parse(program_stream);
    if (!module_exp)
        return Unexpected(PROPAGATE(module_exp));

    Instance& kernel = parent.getKernel();
    Export& sys_call_handler = kernel.exports_.find("sys_call_handler")->second;

    std::shared_ptr<SysCall> sys_call_op =
        std::make_shared<SysCall>(sys_call_handler.func_idx);

    Function sys_call(std::move(sys_call_op), 6,
                      {int32_t(0), int32_t(0), int32_t(0), int32_t(0),
                       int32_t(0), int32_t(0)},
                      sys_call_handler.signature);

    Expected<std::shared_ptr<Instance>> instance_result =
        Instance::create(*module_exp, {{"sys.call", sys_call}});
    if (!instance_result)
        return Unexpected(PROPAGATE(instance_result));

    instance_result->get()->parent_ = &parent;
    return instance_result;
}

Expected<void> Instance::call(const std::string& name) {
    Expected<void> result = call(name, FunctionType::Empty());
    if (!result)
        return Unexpected(PROPAGATE(result));

    return {};
};

Expected<void> Instance::call(const std::string& name, int32_t a) {
    active_context_->pushI32(a);

    fmt::println("{}", FunctionType::ConsumerI32().toString());
    Expected<void> result = call(name, FunctionType::ConsumerI32());
    if (!result)
        return Unexpected(PROPAGATE(result));

    return {};
}

Expected<void> Instance::call(const std::string& name, int32_t a, int32_t b) {
    active_context_->pushI32(a);
    active_context_->pushI32(b);

    Expected<void> result = call(name, FunctionType::ConsumerI32I32());
    if (!result)
        return Unexpected(PROPAGATE(result));

    return {};
}

Expected<int32_t> Instance::callRet(const std::string& name, int32_t a,
                                    int32_t b) {
    active_context_->pushI32(a);
    active_context_->pushI32(b);

    Expected<void> result = call(name, FunctionType::ProducerI32I32());
    if (!result)
        return Unexpected(PROPAGATE(result));

    return active_context_->popI32();
}

Instance::Instance(GlobalState&& store,
                   std::unordered_map<std::string, Export>&& exports,
                   const grammar::Module& module)
    : state_(std::move(store)), exports_(std::move(exports)) {
    ContextManager& mgr = ContextManager::instance();

    size_t context_id = mgr.createContext();
    active_context_ = mgr.getContext(context_id).get();

    if (module.hasStartSection()) {
        uint32_t start_function = module.getStartSection().getFunctionIdx();
        active_context_->setRunState(Context::RunState::running);
        invoke(start_function);
    }
    active_context_->setRunState(Context::RunState::rdy);
}

Expected<void> Instance::call(const std::string& name,
                              const FunctionType& func_type) {
    auto it = exports_.find(name);
    if (it == exports_.end())
        return Unexpected(ERROR(fmt::format(
            "call failed: function '{}' is not exported by the module", name)));
    const Export& exp = it->second;

    size_t signature = std::hash<FunctionType>()(func_type);
    if (exp.signature != signature) {
        return Unexpected(ERROR(
            fmt::format("call failed: signature mismatch when invoking '{}';  "
                        "expected signature '{}' but found '{}'",
                        name, exp.func_type.toString(), func_type.toString())));
    }

    if (active_context_->getRunState() != Context::RunState::rdy)
        return Unexpected(
            ERROR("call failed: stack is not in a runnable state"));

    invoke(exp.func_idx);

    return {};
}

void Instance::invoke(size_t func_idx) {
    Call call(func_idx, UINT32_MAX);
    run(call);
}

void Instance::invokeIndirect(uint32_t element_idx, size_t signature) {
    active_context_->pushI32(element_idx);
    CallIndirect call_indirect(signature);
    run(call_indirect);
}

void Instance::run(OperationBase& operation) {
    Context::RunState old_state = active_context_->getRunState();
    active_instance_ = this;
    active_context_->pushReturn(nullptr);

    Continuation continuation = operation.action(*active_instance_);
    while (continuation) {
        continuation = continuation->action(*active_instance_);
        if (!continuation)
            continuation = active_instance_->active_context_->popReturn();
    }

    active_context_->setRunState(old_state);
}

/*********************/
/* SysCall Operation */
/*********************/

Instance::SysCall::SysCall(uint32_t sys_call_handler_idx)
    : sys_call_handler_idx_(sys_call_handler_idx) {}

Continuation Instance::SysCall::action(Instance& instance) {
    Context& instance_ctx = instance.getActiveContext();
    int32_t n = std::get<int32_t>(instance_ctx.getLocal(0));
    int32_t a1 = std::get<int32_t>(instance_ctx.getLocal(1));
    int32_t a2 = std::get<int32_t>(instance_ctx.getLocal(2));
    int32_t a3 = std::get<int32_t>(instance_ctx.getLocal(3));
    int32_t a4 = std::get<int32_t>(instance_ctx.getLocal(4));
    int32_t a5 = std::get<int32_t>(instance_ctx.getLocal(5));

    Instance& kernel = instance.getKernel();

    Context& kernel_ctx = kernel.getActiveContext();
    kernel_ctx.pushI32(n);
    kernel_ctx.pushI32(a1);
    kernel_ctx.pushI32(a2);
    kernel_ctx.pushI32(a3);
    kernel_ctx.pushI32(a4);
    kernel_ctx.pushI32(a5);

    instance.switchToKernel();

    const Operation& epilogue = createEpilogue(instance);
    kernel_ctx.pushReturn(epilogue.get());
    Function syscall_handler =
        kernel.getGlobalState().getFunction(sys_call_handler_idx_);
    return syscall_handler.enterFrame(kernel_ctx);
}

Instance::SysCall::Epilogue::Epilogue(size_t id, Instance& suspended_instance,
                                      SysCall& parent)
    : id_(id), suspended_instance_(suspended_instance), parent_(parent) {}

Continuation Instance::SysCall::Epilogue::action(Instance& kernel) {
    Context& kernel_ctx = kernel.getActiveContext();

    Function syscall_handler =
        kernel.getGlobalState().getFunction(parent_.sys_call_handler_idx_);
    syscall_handler.leaveFrame(kernel_ctx);

    Context& proc_ctx = suspended_instance_.getActiveContext();
    if (proc_ctx.getRunState() == Context::RunState::running) {
        int32_t result = kernel_ctx.popI32(); // else this is the exit code
        proc_ctx.pushI32(result);
        kernel.switchToInstance(suspended_instance_);
    }

    return nullptr;
}

const Operation& Instance::SysCall::createEpilogue(Instance& instance) {
    size_t idx;

    if (free_list_.empty()) {
        idx = epilogues_.size();
        epilogues_.push_back(std::make_shared<Epilogue>(idx, instance, *this));
    } else {
        idx = free_list_.top();
        free_list_.pop();
        epilogues_[idx] = std::make_shared<Epilogue>(idx, instance, *this);
    }

    return epilogues_[idx];
}

void Instance::SysCall::destroyEpilogue(size_t idx) {
    epilogues_[idx] = nullptr;
    free_list_.push(idx);
}

} // namespace runtime
