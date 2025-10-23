#include <mutex>

#include "runtime/context_manager.hpp"

namespace runtime {

ContextManager& ContextManager::instance() {
    static const auto CONTEXT_MANAGER = std::make_shared<ContextManager>();
    return *CONTEXT_MANAGER;
}

size_t ContextManager::createContext() {
    std::unique_lock lock(mutex_);

    size_t id = contexts_.size();
    contexts_.emplace_back(std::make_shared<Context>(id), nullptr);
    return id;
}

void ContextManager::prepareContext(size_t id, uint32_t entry_func_idx,
                                    int32_t param1) {
    std::shared_lock lock(mutex_);
    auto& [context, entry] = contexts_[id];

    fmt::println("prepareContext: {}", entry_func_idx);

    static const size_t entry_signature =
        std::hash<FunctionType>()(FunctionType::ConsumerI32());
    entry = std::make_shared<CallIndirect>(entry_signature);

    context->pushI32(param1);
    context->pushI32(entry_func_idx);

    context->pushReturn(nullptr);
    context->pushReturn(entry.get());
    context->setRunState(Context::RunState::rdy);
}

const std::shared_ptr<Context>& ContextManager::getContext(size_t id) {
    std::shared_lock lock(mutex_);
    const auto& [context, _] = contexts_[id];
    return context;
}

} // namespace runtime
