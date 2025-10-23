#include <mutex>

#include "runtime/context_manager.hpp"
#include "runtime/process_manager.hpp"

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

void ProcessManager::runProcess(uint32_t pid, Instance& instance) {
    std::shared_lock lock(mutex_);

    Process& process = processes_[pid];
    process.program->setContext(process.program_ctx);
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
    process.entry = std::make_shared<Call>(entry_func_idx);

    ctx->pushReturn(nullptr);
    ctx->pushReturn(process.entry.get());
}

} // namespace runtime
