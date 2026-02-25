#pragma once

#include <memory>
#include <span>

#include "runtime/operations.hpp"
#include "util/errno.hpp"

constexpr uint32_t TIMER_PORT = 1;
constexpr uint32_t KEYBOARD_PORT = 2;

class DeviceBase;
using Device = std::shared_ptr<DeviceBase>;

class DeviceBase {
public:
    virtual ~DeviceBase() = default;

    virtual bool tick() = 0;

    virtual Errno io(int32_t cmd, std::span<uint8_t>& buffer) = 0;

    template <class Derived> Derived& as() {
        return reinterpret_cast<Derived&>(*this);
    }

protected:
    DeviceBase() {}
};