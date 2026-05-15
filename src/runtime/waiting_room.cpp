#include <mutex>

#include "runtime/waiting_room.hpp"

WaitingRoom& WaitingRoom::instance() {
    static const auto WAITING_ROOM = std::make_shared<WaitingRoom>();
    return *WAITING_ROOM;
}

uint32_t WaitingRoom::notify(uint32_t addr, uint32_t max_waiters) {
    std::unique_lock lock(mutex_);

    auto it = waiters_.find(addr);
    if (it == waiters_.end())
        return 0;

    std::unordered_set<runtime::Context*>& blocked = it->second;
    uint32_t notified = 0;

    for (auto iter = blocked.begin();
         iter != blocked.end() && notified < max_waiters;) {
        iter = blocked.erase(iter);
        ++notified;
    }

    if (blocked.empty())
        waiters_.erase(it);

    return notified;
}

void WaitingRoom::block(runtime::Context& context, uint32_t addr) {
    std::unique_lock lock(mutex_);

    std::unordered_set<runtime::Context*>& blocked = waiters_[addr];
    blocked.insert(&context);
}

bool WaitingRoom::isBlocked(runtime::Context& context, uint32_t addr) {
    std::shared_lock lock(mutex_);

    auto it = waiters_.find(addr);
    if (it == waiters_.end())
        return false;

    return it->second.contains(&context);
}
