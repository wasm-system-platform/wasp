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
        tellg = DEVICE_CMD_OFFSET + 1,
        seekg = DEVICE_CMD_OFFSET + 2,
        write = DEVICE_CMD_OFFSET + 3,
        tellp = DEVICE_CMD_OFFSET + 4,
        seekp = DEVICE_CMD_OFFSET + 5,
    };

    enum class Result : int32_t {
        success = 0,
        invalid_arguments = 1,
        disk_error = 2,
    };

    enum class SeekWhence : int32_t {
        set = 0,
        cur = 1,
        end = 2,
    };

    struct SeekCommand {
        Result result;
        int64_t pos;
        SeekWhence whence;
        uint64_t new_pos;
    } __attribute__((packed));

    // input
    void read(Instance& instance, std::span<uint8_t> buffer);
    void tellg(uint32_t* offset);
    void seekg(Instance& instance, std::span<uint8_t> buffer);

    // output
    void write(const void* src, uint32_t count);
    void tellp(uint32_t* offset);
    void seekp(Instance& instance, std::span<uint8_t> buffer);

    std::fstream disk_;
};