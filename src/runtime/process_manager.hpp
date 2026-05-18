#pragma once

#include <memory>
#include <shared_mutex>
#include <vector>

#include "runtime/instance.hpp"
#include "runtime/process.hpp"
#include "util/errno.hpp"

namespace runtime {

class ProcessManager {
public:
    ProcessManager(Kernel& kernel) : kernel_(kernel) {}

    Errno createProcess(Instance& instance,
                        std::span<const char>& program_bytes, uint32_t& pid);
    Errno runProcess(uint32_t pid, Instance& instance, uint32_t execve_stack);
    Errno cloneProcess(uint32_t pid, uint32_t& clone_pid);
    Errno resumeProcess(uint32_t pid, Kernel& kernel, int32_t retval);
    Errno getProcess(uint32_t pid, std::shared_ptr<Process>& proc_out);

    Errno readMemory(uint32_t pid, uint32_t kbuf, uint32_t pbuf,
                     uint32_t count);
    Errno readMemoryCString(uint32_t pid, uint32_t kbuf, uint32_t pbuf,
                            uint32_t maxlen);

    Errno writeMemory(uint32_t pid, uint32_t kbuf, uint32_t pbuf,
                      uint32_t count);

    const std::shared_ptr<runtime::ProcessManager>& getContext(size_t id);

private:
    /*
    struct Process {
        std::shared_ptr<Instance> kernel;
        std::shared_ptr<Instance> program;
        std::shared_ptr<Context> kernel_ctx;
        std::shared_ptr<Context> program_ctx;
        Operation entry;
    };
    */
    Kernel& kernel_;

    std::vector<std::shared_ptr<Process>> processes_;
    std::stack<uint32_t> free_list_;

    std::shared_mutex mutex_;

    uint32_t allocateProcessId();
    void freeProcessId(uint32_t pid);
};

} // namespace runtime
