#include "runtime/kernel.hpp"
#include "hw/wasm_disk.hpp"
#include "runtime/context_manager.hpp"
#include "devices/keyboard.hpp"
#include <iostream>

namespace runtime {

Expected<std::shared_ptr<Kernel>>
Kernel::create(const std::string& kernel_path, const std::string& rootfs_path) {
    std::ifstream kernel_fstream;
    kernel_fstream.open("kernel.wasm", std::ios::in | std::ios::binary);

    // parse module
    Expected<grammar::Module> module_exp = grammar::Module::parse(kernel_fstream);
    if (!module_exp)
        return Unexpected(PROPAGATE(module_exp));

    // create imports to be supplied
    Expected<Imports> imports_exp = Kernel::createImports(rootfs_path);
    if (!imports_exp)
        return Unexpected(PROPAGATE(imports_exp));

    // create global state (functions, memories, debug info, ...)
    Expected<GlobalState> global_state_exp = GlobalState::create(*module_exp, *imports_exp);
    if (!global_state_exp)
        return Unexpected(PROPAGATE(global_state_exp));

    // create exports (entry points)
    Expected<std::shared_ptr<Exports>> exports_exp = Instance::createExports(*module_exp);
    if (!exports_exp)
        return Unexpected(PROPAGATE(exports_exp));

    class Builder : public Kernel {
    public:
        Builder(GlobalState&& global_state, std::shared_ptr<Exports>& exports, const grammar::Module& module)
            : Kernel(std::move(global_state), exports, module) {}
    };
    std::shared_ptr<Kernel> kernel = std::make_shared<Builder>(std::move(*global_state_exp), *exports_exp, *module_exp);

    // check if all handlers are there (interrupt, syscall, page fault)
    Expected<void> result = kernel->validateExports();
    if (!result)
        return Unexpected(PROPAGATE(result));

    // initialize support hw
    result = kernel->setupInterruptController();
    if (!result)
        return Unexpected(PROPAGATE(result));

    result = kernel->setupMMU();
    if (!result)
        return Unexpected(PROPAGATE(result));

    result = kernel->setupDeviceManager();
    if (!result)
        return Unexpected(PROPAGATE(result));

    result = kernel->setupProcessManager();
    if (!result)
        return Unexpected(PROPAGATE(result));

    return kernel;
}

bool Kernel::functionExists(uint32_t idx, size_t signature) {
    if (!getGlobalState().hasFunctionIndirect(idx))
        return false;

    Function& func = getGlobalState().getFunctionIndirect(idx);
    if (func.getSignature() != signature)
        return false;

    return true;
}

Expected<Imports> Kernel::createImports(const std::string& rootfs_path) {
    // Interrupts
    std::function<int32_t(Instance& instance)> interrupts_enable_func =
        [](Instance& instance) -> int32_t {
        return instance.as<Kernel>().controller_->enableInterrupts();
    };
    std::function<int32_t(Instance&)> interrupts_disable_func =
        [](Instance& instance) -> int32_t {
        return instance.as<Kernel>().controller_->disableInterrupts();
    };
    std::function<int32_t(Instance&)> interrupts_enabled_func =
        [](Instance& instance) -> int32_t {
        return instance.as<Kernel>().controller_->getInterruptsEnabled();
    };
    std::function<void(Instance&)> interrupt_idle_func =
        [](Instance& instance) -> void {
        instance.as<Kernel>().controller_->idle();
    };

    Function interrupts_enable =
        Function::createExternal(interrupts_enable_func);
    Function interrupts_disable =
        Function::createExternal(interrupts_disable_func);
    Function interrupts_enabled =
        Function::createExternal(interrupts_enabled_func);
    Function interrupt_idle =
        Function::createExternal(interrupt_idle_func);

    // Terminal
    std::function<void(Instance&, int32_t, int32_t)> terminal_write_func =
        [](Instance& instance, uint32_t buffer_offset, uint32_t len) -> void {
        Memory& memory = instance.getGlobalState().getMemory();
        std::vector<uint8_t> buffer(len);
        
        if (memory.load(buffer_offset, buffer)) {
            std::string_view msg(reinterpret_cast<char*>(buffer.data()), len);
            std::cout << msg << std::flush;
        }
    };

    Function terminal_write = Function::createExternal(terminal_write_func);

    // Console
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t)>
        console_out_func = [](Instance& instance, uint32_t buffer_offset,
                              uint32_t len, uint32_t nwritten_offset) -> int32_t {
        Memory& memory = instance.getGlobalState().getMemory();
        std::vector<uint8_t> buffer(len);
        
        if (!memory.load(buffer_offset, buffer) ||
            !memory.store(nwritten_offset, len)) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        std::string_view msg(reinterpret_cast<char*>(buffer.data()), len);
        std::cout << msg << std::flush;
        return static_cast<int32_t>(Errno::SUCCESS);
    };

    Function console_out = Function::createExternal(console_out_func);

    // Debug
    std::function<void(Instance&, int32_t, int32_t, int32_t)> debug_log_func =
        [](Instance& instance, uint32_t buffer_offset, uint32_t len,
           int32_t level) -> void {
        Memory& memory = instance.getGlobalState().getMemory();
        std::vector<uint8_t> buffer(len);
        
        if (memory.load(buffer_offset, buffer)) {
            std::string_view msg(reinterpret_cast<char*>(buffer.data()), len);
            std::cerr << msg << std::flush;
        }
    };

    Function debug_log = Function::createExternal(debug_log_func);

    // Context
    std::function<int32_t(Instance&)> context_get_func =
        [](Instance& instance) -> uint32_t {
        return instance.getActiveContext().getId();
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t)> context_create_func =
        [](Instance& instance, uint32_t entry_func_idx,
           int32_t param1, uint32_t cid_out_offset) -> int32_t {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.contains(cid_out_offset, sizeof(int32_t))) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        uint32_t cid;

        ContextManager& mgr = ContextManager::instance();
        auto result = mgr.createTrampoline(instance.as<Kernel>(), entry_func_idx, param1, cid);
        
        if (result != Errno::SUCCESS)
            return static_cast<int32_t>(result);

        memory.store(cid_out_offset, cid);
        return static_cast<int32_t>(Errno::SUCCESS);
    };
    std::function<int32_t(Instance&, int32_t)> context_switch_func =
        [](Instance& instance, uint32_t next_id) -> int32_t {
        ContextManager& mgr = ContextManager::instance();
        return static_cast<int32_t>(mgr.switchContext(instance, next_id));
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
        [disk](Instance& instance, uint32_t buffer_offset, uint32_t count) -> int32_t {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.contains(buffer_offset, count)) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        uint8_t* buffer_ptr;
        memory.ptr(buffer_offset, &buffer_ptr);
        return disk->read(buffer_ptr, count);
    };
    std::function<int32_t(Instance&, int32_t)> disk_tellg_func =
        [disk](Instance& instance, uint32_t pos_offset) -> int32_t {
        uint32_t* pos_ptr;

        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.ptr(pos_offset, &pos_ptr)) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        return disk->tellg(pos_ptr);
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t)>
        disk_seekg_func = [disk](Instance& instance, uint32_t pos,
                                 int32_t whence,
                                 uint32_t new_pos_offset) -> int32_t {
        uint32_t* new_pos_ptr;

        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.ptr(new_pos_offset, &new_pos_ptr)) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        return disk->seekg(pos, whence, new_pos_ptr);
    };

    Function disk_read = Function::createExternal(disk_read_func);
    Function disk_tellg = Function::createExternal(disk_tellg_func);
    Function disk_seekg = Function::createExternal(disk_seekg_func);

    std::function<int32_t(Instance&, int32_t, int32_t)> disk_write_func =
        [disk](Instance& instance, uint32_t buffer_offset, uint32_t count) -> int32_t {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.contains(buffer_offset, count)) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        uint8_t* buffer_ptr;
        memory.ptr(buffer_offset, &buffer_ptr);
        return disk->write(buffer_ptr, count);
    };
    std::function<int32_t(Instance&, int32_t)> disk_tellp_func =
        [disk](Instance& instance, uint32_t pos_offset) -> int32_t {
        uint32_t* pos_ptr;

        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.ptr(pos_offset, &pos_ptr)) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        return disk->tellp(pos_ptr);
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t)>
        disk_seekp_func = [disk](Instance& instance, uint32_t pos,
                                 int32_t whence,
                                 uint32_t new_pos_offset) -> int32_t {
        uint32_t* new_pos_ptr;

        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.ptr(new_pos_offset, &new_pos_ptr)) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        return disk->seekp(pos, whence, new_pos_ptr);
    };

    Function disk_write = Function::createExternal(disk_read_func);
    Function disk_tellp = Function::createExternal(disk_tellg_func);
    Function disk_seekp = Function::createExternal(disk_seekg_func);

    // Process
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t)> process_load_func =
        [](Instance& instance, uint32_t program_offset,
           uint32_t program_size, uint32_t pid_offset) -> int32_t {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.contains(program_offset, program_size)) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        char* program_ptr;
        memory.ptr(program_offset, &program_ptr);
        std::span<const char> program_bytes(program_ptr, program_size);

        uint32_t* pid_ptr;
        if (!memory.ptr(pid_offset, &pid_ptr)) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        ProcessManager& proc_manager = *instance.as<Kernel>().proccess_manager_;
        return static_cast<int32_t>(proc_manager.createProcess(instance, program_bytes, *pid_ptr));
    };
    std::function<void(Instance&, int32_t)> process_unload_func =
        [](Instance& instance, uint32_t pid) -> void {};
    std::function<int32_t(Instance&, int32_t, int32_t)> process_run_func =
        [](Instance& instance, uint32_t pid, uint32_t execve_stack) -> int32_t {
        ProcessManager& proc_mgr = *instance.as<Kernel>().proccess_manager_;
        return static_cast<int32_t>(proc_mgr.runProcess(pid, instance, execve_stack));
    };
    std::function<void(Instance&, int32_t)> process_terminate_func =
        [](Instance& instance, uint32_t pid) -> void {
        ProcessManager& proc_mgr = *instance.as<Kernel>().proccess_manager_;
        Instance& proc = proc_mgr.getProcess(pid);
        proc.getActiveContext().setRunState(Context::RunState::rdy);
    };
    std::function<int32_t(Instance&, int32_t, int32_t)> process_clone_func =
        [](Instance& instance, uint32_t pid, uint32_t child_pid_offset) -> int32_t {
        uint32_t* child_pid_ptr;
        
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.ptr(child_pid_offset, &child_pid_ptr)) {
            return static_cast<int32_t>(Errno::BAD_ADDRESS);
        }

        ProcessManager& proc_mgr = *instance.as<Kernel>().proccess_manager_;
        return static_cast<int32_t>(proc_mgr.cloneProcess(pid, *child_pid_ptr));
    };
    std::function<int32_t(Instance&, int32_t, int32_t)> process_resume_func =
        [](Instance& instance, uint32_t pid, int32_t retval) -> int32_t {
        Kernel& kernel = instance.as<Kernel>();
        ProcessManager& proc_mgr = *kernel.proccess_manager_;
        return static_cast<int32_t>(proc_mgr.resumeProcess(pid, kernel, retval));
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)>
        process_read_memory_func = [](Instance& instance, int pid, uint32_t kbuf, uint32_t pbuf, uint32_t count) -> int32_t {
        ProcessManager& proc_mgr = *instance.as<Kernel>().proccess_manager_;
        Errno result = proc_mgr.readMemory(pid, kbuf, pbuf, count);
        return static_cast<int32_t>(result);
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)>
        process_read_memory_cstring_func = [](Instance& instance, int pid, uint32_t kbuf, uint32_t pbuf, uint32_t maxlen) -> int32_t {
        ProcessManager& proc_mgr = *instance.as<Kernel>().proccess_manager_;
        Errno result = proc_mgr.readMemoryCString(pid, kbuf, pbuf, maxlen);
        return static_cast<int32_t>(result);
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)>
        process_write_memory_func = [](Instance& instance, int pid, uint32_t kbuf, uint32_t pbuf, uint32_t count) -> int32_t {
        ProcessManager& proc_mgr = *instance.as<Kernel>().proccess_manager_;
        Errno result = proc_mgr.writeMemory(pid, kbuf, pbuf, count);
        return static_cast<int32_t>(result);
    };

    Function process_load = Function::createExternal(process_load_func);
    Function process_unload = Function::createExternal(process_unload_func);
    Function process_run = Function::createExternal(process_run_func);
    Function process_terminate =
        Function::createExternal(process_terminate_func);
    Function process_clone = Function::createExternal(process_clone_func);
    Function process_resume = Function::createExternal(process_resume_func);
    Function process_read_memory = Function::createExternal(process_read_memory_func);
    Function process_read_memory_cstring = Function::createExternal(process_read_memory_cstring_func);
    Function process_write_memory = Function::createExternal(process_write_memory_func);

    // Devices
    std::function<int64_t(Instance&)> timer_get_time_func =
        [](Instance& instance) -> int64_t {
        //return instance.getTimer().getTime();
        return 0;
    };

    std::function<void(Instance&, int32_t)> timer_set_interval_func =
        [](Instance& instance, uint32_t timout) -> void {
        //instance.getTimer().setInterval(timout);
    };

    Function timer_get_time = Function::createExternal(timer_get_time_func);
    Function timer_set_interval =
        Function::createExternal(timer_set_interval_func);

    // MMU
    std::function<int32_t(Instance&)> mmu_active_table_func =
        [](Instance& instance) -> int32_t {
        return instance.as<Kernel>().mmu_->activeTable();
    };
    std::function<int32_t(Instance&, int32_t)> mmu_create_table_func =
        [](Instance& instance, uint32_t ptable_idx_offset) -> int32_t {
        uint32_t* ptable_idx_ptr;

        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.ptr(ptable_idx_offset, &ptable_idx_ptr)) {
            return static_cast<uint32_t>(Errno::BAD_ADDRESS);
        }

        return static_cast<int32_t>(instance.as<Kernel>().mmu_->createTable(*ptable_idx_ptr));
    };
    std::function<int32_t(Instance&, int32_t)> mmu_destroy_table_func =
        [](Instance& instance, uint32_t ptable_idx) -> int32_t {
        return static_cast<int32_t>(instance.as<Kernel>().mmu_->destroyTable(ptable_idx));
    };
    std::function<int32_t(Instance&, int32_t)> mmu_switch_table_func =
        [](Instance& instance, int32_t table_idx) -> int32_t {
        return static_cast<int32_t>(instance.as<Kernel>().mmu_->loadTable(table_idx));
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)> mmu_map_page_func =
        [](Instance& instance, int32_t ptable_idx, uint32_t virt_addr, uint32_t offset, int32_t prot) -> int32_t {
        return static_cast<int32_t>(instance.as<Kernel>().mmu_->mapPage(ptable_idx, virt_addr, offset, prot));
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)> mmu_unmap_page_func =
        [](Instance& instance, int32_t table_idx, uint32_t virt_addr, uint32_t offset_offset, int32_t prot_offset) -> int32_t {
        uint32_t* offset_ptr;
        int32_t* prot_ptr;
        
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.ptr(offset_offset, &offset_ptr) || !memory.ptr(prot_offset, &prot_ptr))
            return static_cast<int32_t>(Errno::BAD_ADDRESS);

        return static_cast<int32_t>(instance.as<Kernel>().mmu_->unmapPage(table_idx, virt_addr, offset_ptr, prot_ptr));
    };
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)> mmu_get_page_func =
        [](Instance& instance, int32_t table_idx, uint32_t virt_addr, uint32_t offset_offset, int32_t prot_offset) -> int32_t {
        uint32_t* offset_ptr;
        int32_t* prot_ptr;
        
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.ptr(offset_offset, &offset_ptr) || !memory.ptr(prot_offset, &prot_ptr))
            return static_cast<int32_t>(Errno::BAD_ADDRESS);

        return static_cast<int32_t>(instance.as<Kernel>().mmu_->getPage(table_idx, virt_addr, offset_ptr, prot_ptr));
    };

    Function mmu_active_table =
        Function::createExternal(mmu_active_table_func);
    Function mmu_create_table =
        Function::createExternal(mmu_create_table_func);
    Function mmu_destroy_table =
        Function::createExternal(mmu_destroy_table_func);
    Function mmu_switch_table =
        Function::createExternal(mmu_switch_table_func);
    Function mmu_map_page =
        Function::createExternal(mmu_map_page_func);
    Function mmu_unmap_page =
        Function::createExternal(mmu_unmap_page_func);
    Function mmu_get_page =
        Function::createExternal(mmu_get_page_func);

    // Device
    std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)> device_io_func =
        [](Instance& instance, uint32_t port, int32_t cmd, uint32_t buffer_offset, uint32_t buffer_size) -> int32_t {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.contains(buffer_offset, buffer_size))
            return static_cast<int32_t>(Errno::BAD_ADDRESS);

        uint8_t* buffer_ptr;
        memory.ptr(buffer_offset, &buffer_ptr);

        std::span<uint8_t> buffer = std::span<uint8_t>(buffer_ptr, buffer_size);
        return static_cast<int32_t>(DeviceManager::instance().io(port, cmd, buffer));
    };

    Function device_io =
        Function::createExternal(device_io_func);

    return Imports{
        {"interrupt.enable", interrupts_enable},
        {"interrupt.disable", interrupts_disable},
        {"interrupt.is_enabled", interrupts_enabled},
        {"interrupt.idle", interrupt_idle},
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
        {"process.clone", process_clone},
        {"process.resume", process_resume},
        {"process.read_memory", process_read_memory},
        {"process.read_memory_cstring", process_read_memory_cstring},
        {"process.write_memory", process_write_memory},
        {"timer.get_time", timer_get_time},
        {"timer.set_interval", timer_set_interval},
        {"mmu.active_table", mmu_active_table},
        {"mmu.create_table", mmu_create_table},
        {"mmu.destroy_table", mmu_destroy_table},
        {"mmu.switch_table", mmu_switch_table},
        {"mmu.map_page", mmu_map_page},
        {"mmu.unmap_page", mmu_unmap_page},
        {"mmu.get_page", mmu_get_page},
        {"device.io", device_io},
    };
}

Expected<void> Kernel::validateExports() {
    auto it = exports_->find("sys_call_handler");
    if (it == exports_->end())
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

    it = exports_->find("page_fault_handler");
    if (it == exports_->end())
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

    return {};
}

Expected<void> Kernel::setupInterruptController() {
    auto it = exports_->find("interrupt_handler");
    if (it == exports_->end())
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

    controller_ = std::make_unique<InterruptController>(interrupt_handler.func_idx);
    return {};
}

Expected<void> Kernel::setupMMU() {
    auto it = exports_->find("page_fault_handler");
    if (it == exports_->end())
        return Unexpected(ERROR("no page fault handler found"));

    const Export& page_fault_handler = it->second;

    size_t page_fault_handler_sig =
        std::hash<FunctionType>()(FunctionType::ConsumerI32x3());
    if (page_fault_handler.signature != page_fault_handler_sig)
        return Unexpected(
            ERROR(fmt::format("page fault handler has a wrong signature; "
                              "expected signature '{}' but found '{}'",
                              FunctionType::ConsumerI32().toString(),
                              page_fault_handler.func_type.toString())));

    mmu_ = std::make_unique<MemoryManagementUnit>(getGlobalState().getMemory(), page_fault_handler.func_idx);
    return {};
}

Expected<void> Kernel::setupDeviceManager() {
    // plugin generic hardware
    DeviceManager& dev_mgr = DeviceManager::instance();

    dev_mgr.registerController(controller_);
    dev_mgr.plugIn(std::make_shared<Keyboard>(), KEYBOARD_PORT);

    return {};
}

Expected<void> Kernel::setupProcessManager() {
    proccess_manager_ = std::make_unique<ProcessManager>(*this);
    return {};
}

void Kernel::run(OperationBase& entry) {
    active_instance_ = this;

    Context::RunState state = getActiveContext().getRunState();
    assert(state == Context::RunState::rdy);
    getActiveContext().setRunState(Context::RunState::running);
    getActiveContext().getEpilogues().push(nullptr);

    Continuation continuation = entry.action(*this);
    while (continuation) {
        continuation = continuation->action(*active_instance_);

        // check irq
        if (!continuation && controller_->getInterruptsEnabled())
            continuation = controller_->next(active_instance_->getActiveContext());

        // check for epilogues
        if (!continuation)
            continuation = active_instance_->getActiveContext().getEpilogues().pop().get();
    }

    // reset program
    active_instance_->getActiveContext().setRunState(Context::RunState::rdy);
}

}
