#pragma once

#include "runtime/instance.hpp"

#include "util/errno.hpp"

namespace runtime {

namespace checkpoint {

Errno save(Instance& instance, uint32_t checkpoint_offset,
           uint32_t checkpoint_len);

Errno restore(Instance& instance, uint32_t checkpoint_offset,
              uint32_t checkpoint_len);

} // namespace checkpoint

} // namespace runtime