#pragma once

#include "devices/device.hpp"

class Timer : public DeviceBase {
public:
    Timer(uint32_t interrupt_handler_idx);

    runtime::Operation tick(runtime::Instance& instance, uint32_t port,
                            runtime::Operation interrupt) override;

    void setTimout(uint32_t timeout);

    void setInterval(uint32_t timeout);

    void clear();

private:
    struct Timout {
        uint32_t remaining_ = 0;
        uint32_t interval_ = 0;
        bool active_ = false;
        bool interval_mode_ = false;
    };

    Timout t_;
};