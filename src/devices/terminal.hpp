#pragma once

#include <mutex>

#include "devices/device.hpp"

namespace dev {

class Terminal : public DeviceBase {
public:
    Terminal();

    bool tick() override;

    void io(Instance& instance, int32_t cmd,
            std::span<uint8_t> buffer) override;

private:
    static constexpr uint32_t VENDOR_ID = DeviceBase::asciiId("wasp");
    static constexpr uint32_t DEVICE_ID = DeviceBase::asciiId("term");

    enum class Command : int32_t {
        read = DEVICE_CMD_OFFSET + 0,
        write = DEVICE_CMD_OFFSET + 1,
        get_attrs = DEVICE_CMD_OFFSET + 2,
        set_attrs = DEVICE_CMD_OFFSET + 3,
        get_window_size = DEVICE_CMD_OFFSET + 4,
    };

    enum class Result : int32_t {
        success = 0,
        invalid_arguments = 1,
    };

    std::mutex guard_;
    std::deque<char> input_;

    void read(std::span<uint8_t> buffer);
    void write(std::span<uint8_t> buffer);
};

} // namespace dev
