#pragma once

#include <mutex>
#include <thread>

#include "devices/device.hpp"
#include "runtime/interrupts.hpp"

namespace runtime {

enum class BusResult : int32_t {
    success = 0,
    no_device = 1,
    invalid_argument = 2,
};

class DeviceManager {
public:
    static DeviceManager& instance();

    ~DeviceManager();

    void
    registerController(const std::shared_ptr<InterruptController>& controller) {
        const std::lock_guard<std::mutex> guard(guard_);
        controllers_.emplace_back(controller);
    }

    void plugIn(Device device) {
        const std::lock_guard<std::mutex> guard(guard_);
        devices_.push_back(device);
    }

    BusResult io(Instance& instance, uint32_t port, int32_t cmd,
                 std::span<uint8_t> buffer);

private:
    std::vector<std::shared_ptr<InterruptController>> controllers_;
    std::vector<Device> devices_;
    std::mutex guard_;

    std::atomic<bool> running = true;
    std::thread worker;

    DeviceManager();

    void tick(size_t counter);
};

} // namespace runtime
