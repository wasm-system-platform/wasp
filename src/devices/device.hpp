#pragma once

#include <memory>
#include <span>

#include "runtime/operations.hpp"
#include "util/errno.hpp"

using runtime::Instance;

constexpr int32_t DEVICE_CMD_OFFSET = 256;

struct DeviceInfo {
    uint32_t vendor_id;
    uint32_t device_id;
    uint32_t revision;
    uint32_t flags;
} __attribute__((packed));

static_assert(std::is_standard_layout_v<DeviceInfo>);
static_assert(sizeof(DeviceInfo) == 16);

class DeviceBase;
using Device = std::shared_ptr<DeviceBase>;

class DeviceBase {
public:
    virtual ~DeviceBase() = default;

    uint32_t vendorId() const { return vendor_id_; }
    uint32_t deviceId() const { return device_id_; }

    virtual bool tick() = 0;

    virtual void io(Instance& instance, int32_t cmd,
                    std::span<uint8_t> buffer) = 0;

    template <class Derived> Derived& as() {
        return reinterpret_cast<Derived&>(*this);
    }

protected:
    DeviceBase(uint32_t vendor_id, uint32_t device_id)
        : vendor_id_(vendor_id), device_id_(device_id) {}

private:
    uint32_t vendor_id_;
    uint32_t device_id_;
};
