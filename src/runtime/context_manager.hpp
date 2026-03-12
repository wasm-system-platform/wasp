#pragma once

#include <shared_mutex>

#include "runtime/context.hpp"
#include "util/errno.hpp"

namespace runtime {

class Kernel;

class ContextManager {
public:
    static ContextManager& instance();

    Errno createTrampoline(Kernel& kernel, uint32_t entry_func_idx,
                           int32_t param1, uint32_t& out_id);
    Errno destroyContext(uint32_t id);
    Errno cloneContext(uint32_t cid, uint32_t& clone_cid_out);
    Errno switchContext(Instance& instance, uint32_t next_id);

    Context& createEmpty();
    Context& getContext(uint32_t id) { return *contexts_[id]; }

private:
    std::vector<std::shared_ptr<Context>> contexts_;
    std::stack<uint32_t> free_list_;

    std::shared_mutex mutex_;

    uint32_t allocateContextId();
    void freeContextId(uint32_t id);
};

} // namespace runtime
