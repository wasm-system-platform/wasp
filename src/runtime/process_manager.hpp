#pragma once

#include <memory>
#include <shared_mutex>
#include <vector>

#include "runtime/instance.hpp"

namespace runtime {

class ProcessManager {
public:
    static ProcessManager& instance();

    uint32_t createProcess();
    void runProcess(uint32_t pid, Instance& instance);
    void loadProgram(uint32_t pid, Instance& kernel, Instance& program,
                     uint32_t entry_func_idx);
    Instance& getProcess(uint32_t pid);

    const std::shared_ptr<runtime::ProcessManager>& getContext(size_t id);

private:
    struct Process {
        std::shared_ptr<Instance> kernel;
        std::shared_ptr<Instance> program;
        std::shared_ptr<Context> kernel_ctx;
        std::shared_ptr<Context> program_ctx;
        Operation entry;
    };

    std::vector<Process> processes_;
    std::stack<uint32_t> free_list_;

    std::shared_mutex mutex_;
};

} // namespace runtime
