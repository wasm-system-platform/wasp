#include <mutex>

#include "runtime/context_manager.hpp"
#include "runtime/kernel.hpp"

namespace runtime {

ContextManager& ContextManager::instance() {
    static const auto CONTEXT_MANAGER = std::make_shared<ContextManager>();
    return *CONTEXT_MANAGER;
}

Context& ContextManager::createEmpty() {
    uint32_t id = allocateContextId();
    contexts_[id] = std::make_shared<Context>(id);
    Context& ctxt = *contexts_[id];
    ctxt.setRunState(Context::RunState::rdy);
    return ctxt;
}

Errno ContextManager::createTrampoline(Kernel& kernel, uint32_t entry_func_idx, int32_t param1, uint32_t& out_id) {
    static const size_t entry_signature =
        std::hash<FunctionType>()(FunctionType::ConsumerI32());
    static const std::shared_ptr<CallIndirect> entry
        = std::make_shared<CallIndirect>(entry_signature);
    
    if (!kernel.functionExists(entry_func_idx, entry_signature)) {
        fmt::println("oof");
        return Errno::INVALID_ARGUMENT;
    }

    Context& ctxt = createEmpty();
    out_id = ctxt.getId();
    
    ctxt.pushI32(param1);
    ctxt.pushI32(entry_func_idx);

    ctxt.getEpilogues().push(nullptr);
    ctxt.getEpilogues().push(entry);
    ctxt.setRunState(Context::RunState::suspended);

    return Errno::SUCCESS;
};

Errno ContextManager::destroyContext(uint32_t id) {
    if (id >= contexts_.size() || !contexts_[id])
        return Errno::INVALID_ARGUMENT;

    freeContextId(id);
    return Errno::SUCCESS;
}

Errno ContextManager::cloneContext(uint32_t id, uint32_t& clone_id_out) {
    if (id >= contexts_.size() || !contexts_[id])
        return Errno::INVALID_ARGUMENT;

    Context& ctx = *contexts_[id];

    clone_id_out = allocateContextId();
    contexts_[clone_id_out] = std::make_shared<Context>(ctx, id);

    return Errno::SUCCESS;
}

Errno ContextManager::switchContext(Instance& instance, uint32_t next_id) {
    if (next_id >= contexts_.size() || !contexts_[next_id])
        return Errno::INVALID_ARGUMENT;

    instance.setActiveContext(*contexts_[next_id]);
    return Errno::SUCCESS;
}

uint32_t ContextManager::allocateContextId() {
    std::unique_lock lock(mutex_);

    uint32_t id;
    if (free_list_.empty()) {
        id = contexts_.size();
        contexts_.push_back({});
    } else {
        id = free_list_.top();
        free_list_.pop();
    }

    return id;
}

void ContextManager::freeContextId(uint32_t id) {
    std::unique_lock lock(mutex_);
    contexts_[id] = nullptr;
    free_list_.push(id);
}

} // namespace runtime
