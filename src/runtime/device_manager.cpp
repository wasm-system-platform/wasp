#include "runtime/device_manager.hpp"
#include "runtime/interrupts.hpp"

namespace runtime {

DeviceManager& DeviceManager::instance() {
    static DeviceManager dev_mgr;
    return dev_mgr;
}

DeviceManager::DeviceManager() {
    worker = std::thread([this] {
        size_t counter = 0;
        while (running.load()) {
            tick(counter++);
            std::this_thread::sleep_for(std::chrono::microseconds(1));
        }
    });
};

DeviceManager::~DeviceManager() {
    running.store(false);
    if (worker.joinable())
        worker.join();
}

Errno DeviceManager::io(uint32_t port, int32_t cmd,
                        std::span<uint8_t>& buffer) {
    const std::lock_guard<std::mutex> guard(guard_);

    auto it = devices_.find(port);
    if (it == devices_.end()) {
        return Errno::INVALID_ARGUMENT;
    }

    return it->second->io(cmd, buffer);
}

void DeviceManager::tick(size_t counter) {
    const std::lock_guard<std::mutex> guard(guard_);
    if (controllers_.empty())
        return;

    InterruptController& controller =
        *controllers_[counter % controllers_.size()];
    for (const auto& [port, device] : devices_) {
        if (device->tick())
            controller.interrupt(port);
    }
}

} // namespace runtime
