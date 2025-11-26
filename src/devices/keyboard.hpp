#pragma once

#include "devices/device.hpp"

class Keyboard : public DeviceBase {
public:
    Keyboard(uint32_t interrupt_handler_idx);

    runtime::Operation tick(runtime::Instance& instance, uint32_t port,
                            runtime::Operation interrupt) override;

    char getInput();

private:
    std::stack<char> intput_;
};