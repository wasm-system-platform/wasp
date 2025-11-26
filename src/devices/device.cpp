#include "devices/device.hpp"

DeviceBase::DeviceBase(uint32_t interrupt_handler_idx)
    : handler_call(
          std::make_shared<runtime::Call>(interrupt_handler_idx, UINT32_MAX)) {}
