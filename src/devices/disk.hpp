#pragma once

#include <cstdint>
#include <fstream>

#include "devices/device.hpp"
#include "util/error_handling.hpp"

class Disk : public DeviceBase {
public:
    static Expected<Device> create(const std::string& path);

    bool tick() override { return false; }

    void io(Instance& instance, int32_t cmd,
            std::span<uint8_t> buffer) override;

protected:
    Disk(std::fstream&& disk);

private:
    static constexpr uint32_t VENDOR_ID = DeviceBase::asciiId("wasp");
    static constexpr uint32_t DEVICE_ID = DeviceBase::asciiId("disk");

    enum class Command : int32_t {
        read = DEVICE_CMD_OFFSET + 0,
        write = DEVICE_CMD_OFFSET + 1,
        flush = DEVICE_CMD_OFFSET + 2,
        get_size = DEVICE_CMD_OFFSET + 3,
    };

    enum class Result : int32_t {
        success = 0,
        invalid_arguments = 1,
        disk_error = 2,
    };

    // impl
    void read(Instance& instance, std::span<uint8_t> buffer);
    void write(Instance& instance, std::span<uint8_t> buffer);
    void flush(Instance& instance, std::span<uint8_t> buffer);
    void getSize(Instance& instance, std::span<uint8_t> buffer);

    std::fstream disk_;
};