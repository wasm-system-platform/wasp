#pragma once

#include "runtime/operations/operations_control.hpp"
#include "runtime/operations/operations_impl.hpp"

namespace runtime {

inline Continuation IfThen::impl(IfThen& if_then, Instance& instance,
                                 Operation next, Value cond) {
    if (cond.i32)
        return if_then.then_->action(instance);

    return next->action(instance);
}

inline Continuation IfElse::impl(IfElse& if_else, Instance& instance,
                                 Value cond) {
    if (cond.i32)
        return if_else.then_->action(instance);

    return if_else.else_->action(instance);
}

inline Continuation BranchIf::impl(BranchIf& br_if, Instance& instance,
                                   Continuation next, Value cond) {
    (void)instance;

    TRACE_VERBOSE("[{:3}] {}: br_if --> {}: (cond={}) -> ()",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      br_if.addr_),
                  br_if.target_->getFormattedAddress(instance), cond.i32);

    if (cond.i32)
        return br_if.target_->action(instance);

    return next;
}

} // namespace runtime
