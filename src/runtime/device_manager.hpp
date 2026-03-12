#pragma once

#include <mutex>
#include <span>

#include "devices/device.hpp"
#include "runtime/interrupts.hpp"
#include "util/errno.hpp"

namespace runtime {

class DeviceManager {
public:
    static DeviceManager& instance();

    ~DeviceManager();

    void
    registerController(const std::shared_ptr<InterruptController>& controller) {
        const std::lock_guard<std::mutex> guard(guard_);
        controllers_.emplace_back(controller);
    }

    void plugIn(Device&& device, uint32_t port) {
        const std::lock_guard<std::mutex> guard(guard_);
        devices_.emplace(port, std::move(device));
    }

    Errno io(uint32_t port, int32_t cmd, std::span<uint8_t>& buffer);

private:
    std::vector<std::shared_ptr<InterruptController>> controllers_;
    std::unordered_map<uint32_t, Device> devices_;
    std::mutex guard_;

    std::atomic<bool> running = true;
    std::thread worker;

    DeviceManager();

    void tick(size_t counter);
};

} // namespace runtime
