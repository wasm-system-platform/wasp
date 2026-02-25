#include "runtime/process_manager.hpp"
#include "runtime/context_manager.hpp"
#include "runtime/kernel.hpp"
#include "hw/mmu.hpp"
#include <mutex>

namespace runtime {

Errno ProcessManager::createProcess(Instance& instance, const std::string& program, uint32_t& pid) {
    pid = allocateProcessId();

    Expected<std::shared_ptr<Process>> process_exp = Process::create(program, instance, pid);
    if (!process_exp) {
        fmt::println("error: {}", process_exp.error().toString());
        freeProcessId(pid);
        return Errno::INVALID_ARGUMENT;
    }

    processes_[pid] = std::move(*process_exp);

    return Errno::SUCCESS;
}

Errno ProcessManager::runProcess(uint32_t pid, Instance& instance, uint32_t execve_stack) {
    if (pid >= processes_.size())
        return Errno::INVALID_ARGUMENT;

    std::shared_ptr<Process>& proc = processes_[pid];
    if (!proc)
        return Errno::INVALID_ARGUMENT;

    ContextManager& ctxt_manager = ContextManager::instance();    
    Context& ctxt = ctxt_manager.createEmpty();

    ctxt.getEpilogues().push(nullptr);
    ctxt.getEpilogues().push(proc->getEntry());
    ctxt.setRunState(Context::RunState::running);

    proc->setExecveStack(execve_stack);
    proc->setActiveContext(ctxt);
    instance.as<Kernel>().switchToInstance(*proc);
    return Errno::SUCCESS;
}

Errno ProcessManager::cloneProcess(uint32_t pid, uint32_t& clone_pid) {
    // Invalid pid
    if (pid >= processes_.size())
        return Errno::INVALID_ARGUMENT;

    std::shared_ptr<Process> original = processes_[pid];
    if (!original)
        return Errno::INVALID_ARGUMENT;

    Context& original_ctx = original->getActiveContext();
    if (original_ctx.getRunState() != Context::RunState::in_syscall) {
        fmt::println("{}: impl error?", __PRETTY_FUNCTION__);
        __builtin_trap();
        return Errno::INVALID_ARGUMENT;
    }

    clone_pid = allocateProcessId();
    Errno result = original->clone(clone_pid, processes_[clone_pid]);
    if (result != Errno::SUCCESS)
        freeProcessId(clone_pid);

    return result;
}

Errno ProcessManager::resumeProcess(uint32_t pid, Kernel& kernel) {
    if (pid >= processes_.size())
        return Errno::INVALID_ARGUMENT;

    std::shared_ptr<Process>& proc = processes_[pid];
    if (!proc)
        return Errno::INVALID_ARGUMENT;

    Context& proc_ctx = proc->getActiveContext();
    if (proc_ctx.getRunState() != Context::RunState::in_syscall) {
        fmt::println("{}: impl error?", __PRETTY_FUNCTION__);
        __builtin_trap();
        return Errno::INVALID_ARGUMENT;
    }
    proc_ctx.setRunState(Context::RunState::running);

    kernel.switchToInstance(*proc);
    kernel.getActiveContext().getEpilogues().push(nullptr);
    return Errno::SUCCESS;
}

Instance& ProcessManager::getProcess(uint32_t pid) {
    return *processes_[pid];
}

Errno ProcessManager::readMemory(uint32_t pid, uint32_t kbuf, uint32_t pbuf, uint32_t count) {
    std::shared_lock lock(mutex_);

    if (pid >= processes_.size()) {
        fmt::println("Invalid PID: {}", pid);
        return Errno::INVALID_ARGUMENT;
    }

    std::shared_ptr<Process>& process = processes_[pid];

    std::vector<uint8_t>& kernel_memory = kernel_.getGlobalState().getMemory();
    std::vector<uint8_t>& proc_memory = process->getGlobalState().getMemory();

    if (kbuf >= kernel_memory.size() ||
        count > kernel_memory.size() - kbuf) {
        fmt::println("Invalid kernel buffer range: kbuf=0x{:08X}, count={}", kbuf, count);
        return Errno::INVALID_ARGUMENT;
    }

    std::vector<uint8_t> tmp(count);
    MemoryManagementUnit& mmu = kernel_.getMMU();

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

    std::shared_ptr<Process>& process = processes_[pid];

    std::vector<uint8_t>& kernel_memory = kernel_.getGlobalState().getMemory();
    std::vector<uint8_t>& proc_memory = process->getGlobalState().getMemory();

    if (kbuf >= kernel_memory.size() ||
        maxlen > kernel_memory.size() - kbuf)
        return Errno::INVALID_ARGUMENT;

    MemoryManagementUnit& mmu = kernel_.getMMU();

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

    std::shared_ptr<Process>& process = processes_[pid];

    std::vector<uint8_t>& kernel_memory = kernel_.getGlobalState().getMemory();
    std::vector<uint8_t>& proc_memory = process->getGlobalState().getMemory();

    if (kbuf >= kernel_memory.size() ||
        count > kernel_memory.size() - kbuf) {
        fmt::println("Invalid kernel buffer range: kbuf=0x{:08X}, count={}", kbuf, count);
        return Errno::INVALID_ARGUMENT;
    }

    std::vector<uint8_t> tmp(count);
    std::memcpy(tmp.data(), &kernel_memory[kbuf], count);

    MemoryManagementUnit& mmu = kernel_.getMMU();

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

uint32_t ProcessManager::allocateProcessId() {
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

void ProcessManager::freeProcessId(uint32_t pid) {
    std::unique_lock lock(mutex_);
    processes_[pid] = nullptr;
    free_list_.push(pid);
}

} // namespace runtime
