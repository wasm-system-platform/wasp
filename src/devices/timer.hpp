#pragma once

#include <chrono>

#include "devices/device.hpp"

class Timer : public DeviceBase {
public:
    Timer() : DeviceBase(VENDOR_ID, DEVICE_ID) {}

    bool tick() override;

    void io(Instance& instance, int32_t cmd,
            std::span<uint8_t> buffer) override;

private:
    static constexpr uint32_t VENDOR_ID = DeviceBase::asciiId("wasp");
    static constexpr uint32_t DEVICE_ID = DeviceBase::asciiId("tmr\0");

    enum class Command : int32_t {
        start = DEVICE_CMD_OFFSET + 0,
        stop = DEVICE_CMD_OFFSET + 1,
        set = DEVICE_CMD_OFFSET + 2,
        get_time = DEVICE_CMD_OFFSET + 3,
    };

    enum class Result : int32_t {
        success = 0,
        invalid_arguments = 1,
    };

    void start(std::span<uint8_t> buffer);
    void stop(std::span<uint8_t> buffer);
    void set(std::span<uint8_t> buffer);
    void getTime(std::span<uint8_t> buffer);

    bool running_ = false;

    uint64_t accumulated_ns_;
    uint64_t interval_ns_;

    uint64_t max_fires_;
    uint64_t fires_done_;

    std::chrono::time_point<std::chrono::steady_clock> last_tick_;
    std::mutex guard_;
};