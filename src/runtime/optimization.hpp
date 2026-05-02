#pragma once

#include "runtime/operations.hpp"

namespace runtime {

bool canMerge(const Operation& first, const Operation& second);

Operation merge(Operation& first, Operation& second);

void printStats();

} // namespace runtime
