#pragma once

#include <list>
#include <mutex>

#include "devices/device.hpp"

class Keyboard : public DeviceBase {
public:
    Keyboard();

    bool tick() override;

    Errno io(int32_t cmd, std::span<uint8_t>& buffer) override;

private:
    enum class Command { get_input = 1 };

    std::mutex guard_;
    std::list<char> input_;

    Errno getInput(std::span<uint8_t>& buffer);
};