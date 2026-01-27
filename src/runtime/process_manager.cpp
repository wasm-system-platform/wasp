#include <mutex>

#include "runtime/context_manager.hpp"
#include "runtime/process_manager.hpp"
#include "hw/mmu.hpp"

namespace runtime {

ProcessManager& ProcessManager::instance() {
    static ProcessManager manager;
    return manager;
}

uint32_t ProcessManager::createProcess() {
    std::unique_lock lock(mutex_);

    uint32_t pid;
    if (free_list_.empty()) {
        pid = processes_.size();
        processes_.push_back({});
    } else {
        pid = free_list_.top();
        free_list_.pop();
    }

    return pid;
}

void ProcessManager::runProcess(uint32_t pid, Instance& instance, uint32_t execve_stack) {
    std::shared_lock lock(mutex_);

    Process& process = processes_[pid];
    process.program->setContext(process.program_ctx);
    process.program_ctx->setPid(pid);
    process.program_ctx->setRunState(Context::RunState::running);
    process.program_ctx->setExecveStack(execve_stack);
    instance.switchToInstance(*process.program);
}

void ProcessManager::loadProgram(uint32_t pid, Instance& kernel,
                                 Instance& program, uint32_t entry_func_idx) {
    std::shared_lock lock(mutex_);

    ContextManager& ctx_mgr = ContextManager::instance();
    size_t cid = ctx_mgr.createContext();
    const std::shared_ptr<Context>& ctx = ctx_mgr.getContext(cid);

    Process& process = processes_[pid];
    process = {};
    process.kernel = kernel.shared_from_this();
    process.program = program.shared_from_this();
    process.kernel_ctx = program.getActiveContext().shared_from_this();
    process.program_ctx = ctx;
    process.entry = std::make_shared<Call>(entry_func_idx, UINT32_MAX);

    ctx->getEpilogues().push(nullptr);
    ctx->getEpilogues().push(process.entry.get());
}

Instance& ProcessManager::getProcess(uint32_t pid) {
    return *processes_[pid].program;
}

Errno ProcessManager::readMemory(uint32_t pid, uint32_t kbuf, uint32_t pbuf, uint32_t count) {
    std::shared_lock lock(mutex_);

    if (pid >= processes_.size()) {
        fmt::println("Invalid PID: {}", pid);
        return Errno::INVALID_ARGUMENT;
    }

    Process& process = processes_[pid];

    std::vector<uint8_t>& kernel_memory = process.kernel->getGlobalState().getMemory();
    std::vector<uint8_t>& proc_memory = process.program->getGlobalState().getMemory();

    if (kbuf >= kernel_memory.size() ||
        count > kernel_memory.size() - kbuf) {
        fmt::println("Invalid kernel buffer range: kbuf=0x{:08X}, count={}", kbuf, count);
        return Errno::INVALID_ARGUMENT;
    }

    std::vector<uint8_t> tmp(count);
    MemoryManagementUnit& mmu = MemoryManagementUnit::instance();

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t addr = pbuf + i;

        if (addr < VIRT_MEMORY) {
            if (addr >= proc_memory.size()) {
                fmt::println("Invalid process memory access: addr=0x{:08X}", addr);
                return Errno::BAD_ADDRESS;
            }
            tmp[i] = proc_memory[addr];
        } else {
            Errno err = mmu.readI8(addr, reinterpret_cast<int8_t*>(&tmp[i]));
            if (err != Errno::SUCCESS) {
                fmt::println("Invalid process memory access: addr=0x{:08X} count={}", addr, count);
                return err;
            }
        }
    }

    std::memcpy(&kernel_memory[kbuf], tmp.data(), count);
    return Errno::SUCCESS;
}

Errno ProcessManager::readMemoryCString(uint32_t pid, uint32_t kbuf, uint32_t pbuf, uint32_t maxlen) {
    std::shared_lock lock(mutex_);

    if (pid >= processes_.size())
        return Errno::INVALID_ARGUMENT;

    Process& process = processes_[pid];

    auto& kernel_memory = process.kernel->getGlobalState().getMemory();
    auto& proc_memory   = process.program->getGlobalState().getMemory();

    if (kbuf >= kernel_memory.size() ||
        maxlen > kernel_memory.size() - kbuf)
        return Errno::INVALID_ARGUMENT;

    MemoryManagementUnit& mmu = MemoryManagementUnit::instance();

    for (uint32_t i = 0; i < maxlen; ++i) {
        uint8_t byte;
        uint32_t addr = pbuf + i;

        if (addr < VIRT_MEMORY) {
            if (addr >= proc_memory.size())
                return Errno::BAD_ADDRESS;
            byte = proc_memory[addr];
        } else {
            Errno err = mmu.readI8(addr, reinterpret_cast<int8_t*>(&byte));
            if (err != Errno::SUCCESS)
                return err;
        }

        kernel_memory[kbuf + i] = byte;

        if (byte == '\0') {
            return Errno::SUCCESS;
        }
    }

    return Errno::NAME_TOO_LONG;
}

Errno ProcessManager::writeMemory(uint32_t pid, uint32_t kbuf, uint32_t pbuf, uint32_t count) {
    std::shared_lock lock(mutex_);

    if (pid >= processes_.size()) {
        fmt::println("Invalid PID: {}", pid);
        return Errno::INVALID_ARGUMENT;
    }

    Process& process = processes_[pid];

    std::vector<uint8_t>& kernel_memory = process.kernel->getGlobalState().getMemory();
    std::vector<uint8_t>& proc_memory = process.program->getGlobalState().getMemory();

    if (kbuf >= kernel_memory.size() ||
        count > kernel_memory.size() - kbuf) {
        fmt::println("Invalid kernel buffer range: kbuf=0x{:08X}, count={}", kbuf, count);
        return Errno::INVALID_ARGUMENT;
    }

    std::vector<uint8_t> tmp(count);
    std::memcpy(tmp.data(), &kernel_memory[kbuf], count);

    MemoryManagementUnit& mmu = MemoryManagementUnit::instance();

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t addr = pbuf + i;

        if (addr < VIRT_MEMORY) {
            if (addr >= proc_memory.size()) {
                fmt::println("Invalid process memory access: addr=0x{:08X}", addr);
                return Errno::BAD_ADDRESS;
            }
        } else {
            Errno err = mmu.checkAccess(addr, AccessType::WRITE);
            if (err != Errno::SUCCESS) {
                fmt::println("MMU access check failed: addr=0x{:08X}, err={}", addr, static_cast<int>(err));
                return err;
            }
        }
    }

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t addr = pbuf + i;

        if (addr < VIRT_MEMORY) {
            proc_memory[addr] = tmp[i];
        } else {
            mmu.writeI8(addr, tmp[i]);
        }
    }

    return Errno::SUCCESS;
}

} // namespace runtime
