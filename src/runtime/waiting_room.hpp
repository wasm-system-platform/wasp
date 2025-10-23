#pragma once

#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

#include "runtime/context.hpp"

class WaitingRoom {
public:
    static WaitingRoom& instance();

    uint32_t notify(uint32_t addr, uint32_t max_waiters);

    void block(runtime::Context& context, uint32_t addr);

    bool isBlocked(runtime::Context& context, uint32_t addr);

private:
    std::unordered_map<uint32_t, std::unordered_set<runtime::Context*>>
        waiters_;

    std::shared_mutex mutex_;
};
