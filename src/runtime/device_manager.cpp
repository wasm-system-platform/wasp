#include <utility>

#include "runtime/device_manager.hpp"
#include "runtime/interrupts.hpp"

namespace runtime {

enum class BusCommand : int32_t {
    get_device_info = 0,
    num_commands = 256,
};

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

BusResult DeviceManager::io(Instance& instance, uint32_t port, int32_t cmd,
                            std::span<uint8_t> buffer) {
    const std::lock_guard<std::mutex> guard(guard_);

    if (port >= devices_.size() || !devices_[port]) {
        return BusResult::no_device;
    }
    Device& dev = devices_[port];

    switch (cmd) {
    case std::to_underlying(BusCommand::get_device_info): {
        if (buffer.size() < sizeof(DeviceInfo))
            return BusResult::invalid_argument;

        DeviceInfo* info = reinterpret_cast<DeviceInfo*>(buffer.data());
        info->vendor_id = dev->vendorId();
        info->device_id = dev->deviceId();
        return BusResult::success;
    }
    default:
        break;
    }

    dev->io(instance, cmd, buffer);
    return BusResult::success;
}

void DeviceManager::tick(size_t counter) {
    const std::lock_guard<std::mutex> guard(guard_);
    if (controllers_.empty())
        return;

    InterruptController& controller =
        *controllers_[counter % controllers_.size()];
    for (size_t i = 0; i < devices_.size(); i++) {
        if (!devices_[i])
            continue;

        if (devices_[i]->tick())
            controller.interrupt(i);
    }
}

} // namespace runtime
