#pragma once

#include "runtime/instance.hpp"

#include "util/errno.hpp"

namespace runtime {

namespace checkpoint {

Errno create(Instance& instance, std::span<uint8_t> save_out);

Errno restore(Instance& instance, std::span<const uint8_t> save);

} // namespace checkpoint

} // namespace runtime