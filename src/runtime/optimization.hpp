#pragma once

#include "runtime/operations.hpp"

namespace runtime {

bool canMerge(const Operation& first, const Operation& second);

Operation merge(const Operation& first, const Operation& second);

void printStats();

} // namespace runtime
