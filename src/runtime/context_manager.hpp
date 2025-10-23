#pragma once

#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

#include "runtime/context.hpp"

namespace runtime {

class ContextManager {
public:
    static ContextManager& instance();

    size_t createContext();

    void prepareContext(size_t id, uint32_t entry_func_idx, int32_t param1);

    const std::shared_ptr<runtime::Context>& getContext(size_t id);

private:
    std::vector<std::tuple<std::shared_ptr<Context>, Operation>> contexts_;

    std::shared_mutex mutex_;
};

} // namespace runtime
