#pragma once

#include <mutex>

#include "devices/device.hpp"

class Keyboard : public DeviceBase {
public:
    Keyboard();

    bool tick() override;

    void io(Instance& instance, int32_t cmd,
            std::span<uint8_t> buffer) override;

private:
    static constexpr uint32_t VENDOR_ID = 0x77617370;
    static constexpr uint32_t DEVICE_ID = 0x6b796264;

    enum class Command : int32_t {
        get_input = DEVICE_CMD_OFFSET + 0,
    };

    enum class Result : int32_t {
        success = 0,
        invalid_arguments = 1,
    };

    std::mutex guard_;
    std::deque<char> input_;

    void getInput(std::span<uint8_t> buffer);
};