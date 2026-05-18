#include "runtime/process_manager.hpp"
#include "runtime/context_manager.hpp"
#include "runtime/kernel.hpp"
#include <mutex>

namespace runtime {

using hw::mem::VIRT_MEMORY;

Errno ProcessManager::createProcess(Instance& instance,
                                    std::span<const char>& program_bytes,
                                    uint32_t& pid) {
    pid = allocateProcessId();

    Expected<std::shared_ptr<Process>> process_exp =
        Process::create(program_bytes, instance, pid);
    if (!process_exp) {
        fmt::println("error: {}", process_exp.error().toString());
        freeProcessId(pid);
        return Errno::invalid;
    }

    processes_[pid] = std::move(*process_exp);

    return Errno::success;
}

Errno ProcessManager::runProcess(uint32_t pid, Instance& instance,
                                 uint32_t execve_stack) {
    if (pid >= processes_.size())
        return Errno::invalid;

    std::shared_ptr<Process>& proc = processes_[pid];
    if (!proc)
        return Errno::invalid;

    ContextManager& ctxt_manager = ContextManager::instance();
    Context& ctxt = ctxt_manager.createEmpty();

    ctxt.getEpilogues().push(nullptr);
    ctxt.getEpilogues().push(proc->getEntry());
    ctxt.setRunState(Context::RunState::running);

    proc->setExecveStack(execve_stack);
    proc->setActiveContext(ctxt);
    instance.as<Kernel>().switchToInstance(*proc);
    return Errno::success;
}

Errno ProcessManager::cloneProcess(uint32_t pid, uint32_t& clone_pid) {
    // Invalid pid
    if (pid >= processes_.size())
        return Errno::invalid;

    std::shared_ptr<Process> original = processes_[pid];
    if (!original)
        return Errno::invalid;

    Context& original_ctx = original->getActiveContext();
    if (original_ctx.getRunState() != Context::RunState::in_syscall) {
        fmt::println("{}: impl error?", __PRETTY_FUNCTION__);
        __builtin_trap();
        return Errno::invalid;
    }

    clone_pid = allocateProcessId();
    Errno result = original->clone(clone_pid, processes_[clone_pid]);
    if (result != Errno::success)
        freeProcessId(clone_pid);

    return result;
}

Errno ProcessManager::resumeProcess(uint32_t pid, Kernel& kernel,
                                    int32_t retval) {
    if (pid >= processes_.size())
        return Errno::invalid;

    std::shared_ptr<Process>& proc = processes_[pid];
    if (!proc)
        return Errno::invalid;

    Context& proc_ctx = proc->getActiveContext();
    if (proc_ctx.getRunState() != Context::RunState::in_syscall) {
        fmt::println("{}: impl error?", __PRETTY_FUNCTION__);
        __builtin_trap();
        return Errno::invalid;
    }

    proc_ctx.pushI32(retval);
    proc_ctx.setRunState(Context::RunState::running);

    kernel.switchToInstance(*proc);
    return Errno::success;
}

Errno ProcessManager::getProcess(uint32_t pid, std::shared_ptr<Process>& proc_out) {
    if (pid <= processes_.size() || !processes_[pid])
        return Errno::invalid;
    
    proc_out = processes_[pid];
    return Errno::success;
}

Errno ProcessManager::readMemory(uint32_t pid, uint32_t kbuffer_offset,
                                 uint32_t pbuffer_offset, uint32_t count) {
    std::shared_lock lock(mutex_);

    if (pid >= processes_.size()) {
        fmt::println("Invalid PID: {}", pid);
        return Errno::invalid;
    }

    std::shared_ptr<Process>& process = processes_[pid];

    Memory& kernel_memory = kernel_.getGlobalState().getMemory();
    Memory& proc_memory = process->getGlobalState().getMemory();

    std::vector<uint8_t> tbuffer(count);
    MemoryManagementUnit& mmu = kernel_.getMMU();

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t offset = pbuffer_offset + i;

        if (offset < VIRT_MEMORY) {
            if (!proc_memory.load(offset, tbuffer[i])) {
                fmt::println("Invalid process memory access: addr=0x{:08X}",
                             offset);
                return Errno::BAD_ADDRESS;
            }
        } else {
            if (!mmu.load(offset, tbuffer[i])) {
                fmt::println(
                    "Invalid process memory access: addr=0x{:08X} count={}",
                    offset, count);
                return Errno::BAD_ADDRESS;
            }
        }
    }

    kernel_memory.store(kbuffer_offset, tbuffer);
    return Errno::success;
}

Errno ProcessManager::readMemoryCString(uint32_t pid, uint32_t kbuffer_offset,
                                        uint32_t pbuffer_offset,
                                        uint32_t maxlen) {
    std::shared_lock lock(mutex_);

    if (pid >= processes_.size())
        return Errno::invalid;

    std::shared_ptr<Process>& process = processes_[pid];

    Memory& kernel_memory = kernel_.getGlobalState().getMemory();
    Memory& proc_memory = process->getGlobalState().getMemory();
    MemoryManagementUnit& mmu = kernel_.getMMU();

    std::vector<uint8_t> tbuffer(maxlen);

    for (uint32_t i = 0; i <= maxlen; ++i) {
        if (i == maxlen)
            return Errno::NAME_TOO_LONG;

        uint32_t offset = pbuffer_offset + i;

        if (offset < VIRT_MEMORY) {
            if (!proc_memory.load(offset, tbuffer[i]))
                return Errno::BAD_ADDRESS;
        } else {
            if (!mmu.load(offset, tbuffer[i]))
                return Errno::BAD_ADDRESS;
        }

        if (tbuffer[i] == '\0')
            break;
    }

    for (uint32_t i = 0; i <= maxlen; ++i) {
        uint32_t offset = kbuffer_offset + i;

        if (offset < VIRT_MEMORY) {
            if (!kernel_memory.store(offset, tbuffer[i]))
                return Errno::BAD_ADDRESS;
        } else {
            if (!mmu.store(offset, tbuffer[i]))
                return Errno::BAD_ADDRESS;
        }

        if (tbuffer[i] == '\0')
            break;
    }

    return Errno::success;
}

Errno ProcessManager::writeMemory(uint32_t pid, uint32_t kbuffer_offset,
                                  uint32_t pbuffer_offset, uint32_t count) {
    std::shared_lock lock(mutex_);

    if (pid >= processes_.size()) {
        fmt::println("Invalid PID: {}", pid);
        return Errno::invalid;
    }

    std::shared_ptr<Process>& process = processes_[pid];

    Memory& kernel_memory = kernel_.getGlobalState().getMemory();
    Memory& proc_memory = process->getGlobalState().getMemory();
    MemoryManagementUnit& mmu = kernel_.getMMU();

    std::vector<uint8_t> buffer(count);

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t offset = kbuffer_offset + i;

        if (offset < VIRT_MEMORY) {
            if (!kernel_memory.load(offset, buffer[i]))
                return Errno::BAD_ADDRESS;
        } else {
            if (!mmu.load(offset, buffer[i]))
                return Errno::BAD_ADDRESS;
        }
    }

    for (uint32_t i = 0; i < count; ++i) {
        uint32_t offset = pbuffer_offset + i;

        if (offset < VIRT_MEMORY) {
            if (!proc_memory.store(offset, buffer[i]))
                return Errno::BAD_ADDRESS;
        } else {
            if (!mmu.store(offset, buffer[i]))
                return Errno::BAD_ADDRESS;
        }
    }

    return Errno::success;
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
