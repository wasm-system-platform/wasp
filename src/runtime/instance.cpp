#include <fmt/format.h>
#include <sstream>

#include "devices/keyboard.hpp"
#include "hw/wasm_disk.hpp"
#include "hw/mmu.hpp"
#include "runtime/context_manager.hpp"
#include "runtime/instance.hpp"
#include "runtime/handler.hpp"
#include "runtime/process_manager.hpp"
#include "runtime/setjmp.hpp"

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
    std::function<int32_t(Instance & instance)> interrupts_enable_func =
        [](Instance& instance) -> int32_t {
        return instance.getGlobalState().enableInterrupts();
    };
    std::function<int32_t(Instance&)> interrupts_disable_func =
        [](Instance& instance) -> int32_t {
        return instance.getGlobalState().disableInterrupts();
    };
    std::function<int32_t(Instance&)> interrupts_enabled_func =
        [](Instance& instance) -> int32_t {
        return instance.getGlobalState().getInterruptsEnabled();
    };

    Function interrupts_enable =
        Function::createExternal(interrupts_enable_func);
    Function interrupts_disable =
        Function::createExternal(interrupts_disable_func);
    Function interrupts_enabled =
        Function::createExternal(interrupts_enabled_func);

    // Terminal
    std::function<void(Instance&, int32_t, int32_t)> terminal_write_func =
        [](Instance& instance, uint32_t offset, uint32_t len) -> void {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();

        if (offset + len < memory.size()) {
            std::string_view msg(reinterpret_cast<char*>(&memory[offset]), len);
            fmt::print("{}", msg);
        }
    };

    Function terminal_write = Function::createExternal(terminal_write_func);

    // Console
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t)>
        console_out_func = [](Instance& instance, uint32_t buffer,
                              uint32_t count, uint32_t nwritten) -> int32_t {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();

        if (buffer + count >= memory.size() ||
            nwritten + sizeof(int32_t) >= memory.size()) {
            return -1;
        }

        std::string_view msg(reinterpret_cast<char*>(&memory[buffer]), count);
        fmt::print("{}", msg);

        *reinterpret_cast<uint32_t*>(&memory[nwritten]) = count;

        return 0;
    };

    Function console_out = Function::createExternal(console_out_func);

    // Debug
    std::function<void(Instance&, int32_t, int32_t, int32_t)> debug_log_func =
        [](Instance& instance, uint32_t buffer, uint32_t count,
           int32_t level) -> void {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();

        if (buffer + count < memory.size()) {
            std::string_view msg(reinterpret_cast<char*>(&memory[buffer]),
                                 count);
            fmt::print("{}", msg);
        }
    };

    Function debug_log = Function::createExternal(debug_log_func);

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
    Expected<WasmDisk> disk_exp = WasmDisk::create(rootfs_path);
    if (!disk_exp)
        return Unexpected(PROPAGATE(disk_exp));

    std::shared_ptr<WasmDisk> disk =
        std::make_shared<WasmDisk>(std::move(*disk_exp));

    std::function<int32_t(Instance&, int32_t, int32_t)> disk_read_func =
        [disk](Instance& instance, uint32_t buf, uint32_t count) -> int32_t {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
        if (memory.size() < buf + 1) {
            return -1;
        }

        return disk->read(&instance.getGlobalState().getMemory()[buf], count);
    };
    std::function<int32_t(Instance&, int32_t)> disk_tellg_func =
        [disk](Instance& instance, uint32_t offset) -> int32_t {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
        if (memory.size() < offset + sizeof(uint32_t)) {
            return -1;
        }

        return disk->tellg(reinterpret_cast<uint32_t*>(&memory[offset]));
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t)>
        disk_seekg_func = [disk](Instance& instance, uint32_t offset,
                                 int32_t whence,
                                 uint32_t new_offset) -> int32_t {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
        if (memory.size() < new_offset + sizeof(uint32_t)) {
            return -1;
        }

        return disk->seekg(offset, whence,
                           reinterpret_cast<uint32_t*>(&memory[new_offset]));
    };

    Function disk_read = Function::createExternal(disk_read_func);
    Function disk_tellg = Function::createExternal(disk_tellg_func);
    Function disk_seekg = Function::createExternal(disk_seekg_func);

    std::function<int32_t(Instance&, int32_t, int32_t)> disk_write_func =
        [disk](Instance& instance, uint32_t buf, uint32_t count) -> int32_t {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
        if (memory.size() < buf + 1) {
            return -1;
        }

        return disk->write(&instance.getGlobalState().getMemory()[buf], count);
    };
    std::function<int32_t(Instance&, int32_t)> disk_tellp_func =
        [disk](Instance& instance, uint32_t offset) -> int32_t {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
        if (memory.size() < offset + sizeof(uint32_t)) {
            return -1;
        }

        return disk->tellp(reinterpret_cast<uint32_t*>(&memory[offset]));
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t)>
        disk_seekp_func = [disk](Instance& instance, uint32_t offset,
                                 int32_t whence,
                                 uint32_t new_offset) -> int32_t {
        std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
        if (memory.size() < new_offset + sizeof(uint32_t)) {
            return -1;
        }

        return disk->seekp(offset, whence,
                           reinterpret_cast<uint32_t*>(&memory[new_offset]));
    };

    Function disk_write = Function::createExternal(disk_read_func);
    Function disk_tellp = Function::createExternal(disk_tellg_func);
    Function disk_seekp = Function::createExternal(disk_seekg_func);

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
    std::function<int32_t(Instance&, int32_t, int32_t)> process_run_func =
        [](Instance& instance, uint32_t pid, uint32_t execve_stack) -> int32_t {
        ProcessManager& proc_mgr = ProcessManager::instance();
        proc_mgr.runProcess(pid, instance, execve_stack);
        return 0;
    };
    std::function<void(Instance&, int32_t)> process_terminate_func =
        [](Instance& instance, uint32_t pid) -> void {
        ProcessManager& proc_mgr = ProcessManager::instance();
        Instance& proc = proc_mgr.getProcess(pid);
        proc.getActiveContext().setRunState(Context::RunState::rdy);
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)>
        process_read_memory_func = [](Instance& instance, int pid, uint32_t kbuf, uint32_t pbuf, uint32_t count) -> int32_t {
        ProcessManager& proc_mgr = ProcessManager::instance();
        Errno result = proc_mgr.readMemory(pid, kbuf, pbuf, count);
        return static_cast<int32_t>(result);
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)>
        process_read_memory_cstring_func = [](Instance& instance, int pid, uint32_t kbuf, uint32_t pbuf, uint32_t maxlen) -> int32_t {
        ProcessManager& proc_mgr = ProcessManager::instance();
        Errno result = proc_mgr.readMemoryCString(pid, kbuf, pbuf, maxlen);
        return static_cast<int32_t>(result);
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)>
        process_write_memory_func = [](Instance& instance, int pid, uint32_t kbuf, uint32_t pbuf, uint32_t count) -> int32_t {
        ProcessManager& proc_mgr = ProcessManager::instance();
        Errno result = proc_mgr.writeMemory(pid, kbuf, pbuf, count);
        return static_cast<int32_t>(result);
    };

    Function process_load = Function::createExternal(process_load_func);
    Function process_unload = Function::createExternal(process_unload_func);
    Function process_run = Function::createExternal(process_run_func);
    Function process_terminate =
        Function::createExternal(process_terminate_func);
    Function process_read_memory = Function::createExternal(process_read_memory_func);
    Function process_read_memory_cstring = Function::createExternal(process_read_memory_cstring_func);
    Function process_write_memory = Function::createExternal(process_write_memory_func);

    // Devices
    std::function<int64_t(Instance&)> timer_get_time_func =
        [](Instance& instance) -> int64_t {
        return instance.getTimer().getTime();
    };

    std::function<void(Instance&, int32_t)> timer_set_interval_func =
        [](Instance& instance, uint32_t timout) -> void {
        instance.getTimer().setInterval(timout);
    };

    Function timer_get_time = Function::createExternal(timer_get_time_func);
    Function timer_set_interval =
        Function::createExternal(timer_set_interval_func);


    /* MMU */
    std::function<int32_t(Instance&)> mmu_active_table_func =
        [](Instance& instance) -> int32_t {
        return MemoryManagementUnit::instance().activeTable();
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)> mmu_map_page_func =
        [](Instance& instance, int32_t table_idx, uint32_t va, uint32_t pa, int32_t prot) -> int32_t {
        std::vector<uint8_t>& mem = instance.getGlobalState().getMemory();
        if (pa > mem.size()) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }
        uint8_t* page = &mem[pa];

        return static_cast<int32_t>(MemoryManagementUnit::instance().mapPage(table_idx, va, page, prot));
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)> mmu_unmap_page_func =
        [](Instance& instance, int32_t table_idx, uint32_t va, uint32_t pa_ptr, int32_t prot_ptr) -> int32_t {
        std::vector<uint8_t>& mem = instance.getGlobalState().getMemory();
        if (pa_ptr > mem.size() || prot_ptr > mem.size()) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }
        uint8_t** pa = reinterpret_cast<uint8_t**>(&mem[pa_ptr]);
        int* prot = reinterpret_cast<int32_t*>(&mem[prot_ptr]);

        return static_cast<int32_t>(MemoryManagementUnit::instance().unmapPage(table_idx, va, pa, prot));
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)> mmu_get_page_func =
        [](Instance& instance, int32_t table_idx, uint32_t va, uint32_t pa_ptr, int32_t prot_ptr) -> int32_t {
        std::vector<uint8_t>& mem = instance.getGlobalState().getMemory();
        if (pa_ptr > mem.size() || prot_ptr > mem.size()) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }
        uint8_t** pa = reinterpret_cast<uint8_t**>(&mem[pa_ptr]);
        int* prot = reinterpret_cast<int32_t*>(&mem[prot_ptr]);

        return static_cast<int32_t>(MemoryManagementUnit::instance().getPage(table_idx, va, pa, prot));
    };

    Function mmu_active_table =
        Function::createExternal(mmu_active_table_func);
    Function mmu_map_page =
        Function::createExternal(mmu_map_page_func);
    Function mmu_unmap_page =
        Function::createExternal(mmu_unmap_page_func);
    Function mmu_get_page =
        Function::createExternal(mmu_get_page_func);

    Expected<std::shared_ptr<Instance>> instance_exp = Instance::create(
        *module, {
                     {"interrupt.enable", interrupts_enable},
                     {"interrupt.disable", interrupts_disable},
                     {"interrupt.is_enabled", interrupts_enabled},
                     {"console.out", console_out},
                     {"debug.log", debug_log},
                     {"context.get", context_get},
                     {"context.create", context_create},
                     {"context.switch", context_switch},
                     {"disk.read", disk_read},
                     {"disk.tellg", disk_tellg},
                     {"disk.seekg", disk_seekg},
                     {"disk.write", disk_write},
                     {"disk.tellp", disk_tellp},
                     {"disk.seekp", disk_seekp},
                     {"process.load", process_load},
                     {"process.unload", process_unload},
                     {"process.run", process_run},
                     {"process.terminate", process_terminate},
                     {"process.read_memory", process_read_memory},
                     {"process.read_memory_cstring", process_read_memory_cstring},
                     {"process.write_memory", process_write_memory},
                     {"timer.get_time", timer_get_time},
                     {"timer.set_interval", timer_set_interval},
                     {"mmu.active_table", mmu_active_table},
                     {"mmu.map_page", mmu_map_page},
                     {"mmu.unmap_page", mmu_unmap_page},
                     {"mmu.get_page", mmu_get_page},
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

    it = instance.exports_.find("sys_call_handler");
    if (it == instance.exports_.end())
        return Unexpected(ERROR("no system call handler found"));

    const Export& syscall_handler = it->second;

    size_t syscall_handler_sig =
        std::hash<FunctionType>()(FunctionType::ProducerI32x8());
    if (syscall_handler.signature != syscall_handler_sig)
        return Unexpected(
            ERROR(fmt::format("syscall handler has a wrong signature; "
                              "expected signature '{}' but found '{}'",
                              FunctionType::ProducerI32x8().toString(),
                              syscall_handler.func_type.toString())));

    it = instance.exports_.find("page_fault_handler");
    if (it == instance.exports_.end())
        return Unexpected(ERROR("no page fault handler found"));

    const Export& page_fault_handler = it->second;
    size_t page_fault_handler_sig =
        std::hash<FunctionType>()(FunctionType::ConsumerI32x3());
    if (page_fault_handler.signature != page_fault_handler_sig)
        return Unexpected(
            ERROR(fmt::format("page fault handler has a wrong signature; "
                              "expected signature '{}' but found '{}'",
                              FunctionType::ConsumerI32x3().toString(),
                              page_fault_handler.func_type.toString())));

    instance.page_fault_ = std::make_shared<PageFault>(page_fault_handler.func_idx);

    return instance_exp;
}

Expected<std::shared_ptr<Instance>>
Instance::createUserProgram(const std::string& program, Instance& parent) {
    std::stringstream program_stream(program);

    Expected<grammar::Module> module_exp =
        grammar::Module::parse(program_stream);
    if (!module_exp)
        return Unexpected(PROPAGATE(module_exp));

    grammar::Module& module = *module_exp;

    Instance& kernel = parent.getKernel();

    auto it = kernel.exports_.find("sys_call_handler");
    if (it == kernel.exports_.end()) {
        return Unexpected(ERROR("kernel export 'sys_call_handler' not found"));
    }

    Export& sys_call_handler = it->second;

    std::shared_ptr<SysCall> sys_call_op =
        std::make_shared<SysCall>(sys_call_handler.func_idx);

    Function sys_call(std::move(sys_call_op), 7,
                      {int32_t(0), int32_t(0), int32_t(0), int32_t(0),
                       int32_t(0), int32_t(0), int32_t(0)},
                      FunctionType::ProducerI32x7().getSignature());

    std::function<int32_t(Instance&)> sys_execve_stack_func =
        [](Instance& instance) -> int32_t {
        return instance.getActiveContext().getExecveStack();
    };

    Function sys_execve_stack = Function::createExternal(sys_execve_stack_func);

    /* setjmp/longjmp */
    std::shared_ptr<Sigsetjmp> env_sigsetjmp_op = std::make_shared<Sigsetjmp>();
    Sigsetjmp& sigsetjmp = *env_sigsetjmp_op;
    Function env_sigsetjmp(env_sigsetjmp_op, 2, {int32_t(0), int32_t(0)}, FunctionType::ProducerI32x2().getSignature());
    
    std::shared_ptr<Longjmp> env_longjmp_op = std::make_shared<Longjmp>();
    Function env_longjmp(env_longjmp_op, 2, {int32_t(0), int32_t(0)}, FunctionType::ConsumerI32x2().getSignature());
 
    Expected<std::shared_ptr<Instance>> instance_result =
        Instance::create(*module_exp, {{"sys.call", sys_call},
                                       {"sys.execve_stack", sys_execve_stack},
                                    {"env.sigsetjmp", env_sigsetjmp},
                                    {"env.longjmp", env_longjmp},
                                    });
    if (!instance_result)
        return Unexpected(PROPAGATE(instance_result));

    Instance& instance = **instance_result;
    instance.parent_ = &parent;
    instance.page_fault_ = parent.page_fault_;

    it = instance.exports_.find("sigsetjmp_prologue");
    if (it != instance.exports_.end()) {
        Export& sigsetjmp_prologue = it->second;
        size_t sigsetjmp_prologue_sig = FunctionType::ConsumerI32x2().getSignature();

        if (sigsetjmp_prologue_sig == sigsetjmp_prologue.signature) {
            fmt::println("set sigsetjmp prologue");
            sigsetjmp.setPrologueFuncIdx(sigsetjmp_prologue.func_idx);
        } else {
            fmt::println("warning: sigsetjmp prologue has a wrong signature; "
                              "expected signature '{}' but found '{}'",
                              FunctionType::ConsumerI32x2().toString(),
                              sigsetjmp_prologue.func_type.toString());
        }
    }

    it = instance.exports_.find("sigsetjmp_epilogue");
    if (it != instance.exports_.end()) {
        Export& sigsetjmp_epilogue = it->second;
        size_t sigsetjmp_epilogue_sig = FunctionType::ConsumerI32x2().getSignature();

        if (sigsetjmp_epilogue_sig == sigsetjmp_epilogue.signature) {
            fmt::println("set sigsetjmp epilogue");
            sigsetjmp.setEpilogueFuncIdx(sigsetjmp_epilogue.func_idx);
        } else {
            fmt::println("warning: sigsetjmp epilogue has a wrong signature; "
                              "expected signature '{}' but found '{}'",
                              FunctionType::ConsumerI32x2().toString(),
                              sigsetjmp_epilogue.func_type.toString());
        }
    }

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

    Expected<void> result = call(name, FunctionType::ConsumerI32x2());
    if (!result)
        return Unexpected(PROPAGATE(result));

    return {};
}

Expected<int32_t> Instance::callRet(const std::string& name, int32_t a,
                                    int32_t b) {
    active_context_->pushI32(a);
    active_context_->pushI32(b);

    Expected<void> result = call(name, FunctionType::ProducerI32x2());
    if (!result)
        return Unexpected(PROPAGATE(result));

    return active_context_->pop().i32;
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
    active_context_->getEpilogues().push(nullptr);

    Continuation continuation = operation.action(*active_instance_);
    while (continuation) {
        continuation = continuation->action(*active_instance_);
        if (!continuation)
            continuation =
                active_instance_->active_context_->getEpilogues().pop();
    }

    active_context_->setRunState(old_state);
}

} // namespace runtime
