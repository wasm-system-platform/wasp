#pragma once

#include <memory>

#include "runtime/operations.hpp"

constexpr size_t TIMER_PORT = 1;

class DeviceBase;
using Device = std::shared_ptr<DeviceBase>;

class DeviceBase {
public:
    virtual ~DeviceBase() = default;

    virtual runtime::Operation tick(runtime::Instance& instance, uint32_t port,
                                    runtime::Operation interrupt) = 0;

    template <class Derived> Derived& as() {
        return reinterpret_cast<Derived&>(*this);
    }

protected:
    DeviceBase(uint32_t interrupt_handler_idx);

    runtime::Operation handler_call;
};