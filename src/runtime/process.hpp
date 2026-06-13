#pragma once

#include "runtime/instance.hpp"
#include "util/errno.hpp"

namespace runtime {

class Sigsetjmp;

class Process : public TaggedInstance<Process> {
public:
    static Expected<std::shared_ptr<Process>>
    create(std::span<const char>& program_bytes, Instance& instance,
           uint32_t id);

    Kernel& getKernel() { return kernel_; }

    uint32_t getId() const { return id_; }

    void setExecveStack(uint32_t stack) { execve_stack_ = stack; }
    uint32_t getExecveStack() const { return execve_stack_; }

    Errno clone(uint32_t new_id, std::shared_ptr<Process>& proc_out);

    const Operation& getEntry() { return entry_; }

    const std::array<uint8_t, 32>& getKey() const { return key_; }
    std::unordered_set<Operation>& getEpilogueCache() {
        return epilogue_cache_;
    }

protected:
    Process(GlobalState&& global_state, std::shared_ptr<Exports>& exports,
            const grammar::Module& module, Kernel& kernel, uint32_t id)
        : TaggedInstance<Process>(std::move(global_state), exports, module),
          kernel_(kernel), id_(id) {}

    Process(Process& process, uint32_t new_id)
        : TaggedInstance<Process>(process), kernel_(process.kernel_),
          id_(new_id) {}

private:
    Kernel& kernel_;

    uint32_t id_;
    uint32_t execve_stack_ = 0;
    std::array<uint8_t, 32> key_;
    std::unordered_set<Operation> epilogue_cache_;

    Operation entry_;

    static Expected<Imports>
    createImports(Instance& instance);

    Expected<void>
    validateExports();
};

} // namespace runtime
