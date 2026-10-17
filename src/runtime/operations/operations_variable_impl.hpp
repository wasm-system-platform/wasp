#pragma once

#include "runtime/operations/operations_impl.hpp"
#include "runtime/operations/operations_variable.hpp"

namespace runtime {

inline Value LocalGet::impl(LocalGet& local_get, Instance& instance) {
    Value out = instance.getActiveContext().getLocal(local_get.local_idx_);
    TRACE_VERBOSE("[{:3}] {}: local.get {}: () -> (i32={}, i64={})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      local_get.addr_),
                  local_get.local_idx_, local.i32, local.i64);
    return out;
}

inline void LocalSet::impl(LocalSet& local_set, Instance& instance, Value in) {
    instance.getActiveContext().setLocal(local_set.local_idx_, in);
    TRACE_VERBOSE("[{:3}] {}: local.set {}: (i32={}, i64={}) -> ()",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      local_set.addr_),
                  local_set.local_idx_, in.i32, in.i64);
};

inline Value LocalTee::impl(LocalTee& local_tee, Instance& instance, Value in) {
    instance.getActiveContext().setLocal(local_tee.local_idx_, in);
    TRACE_VERBOSE("[{:3}] {}: local.tee {}: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      local_tee.addr_),
                  local_tee.local_idx_, in.i64, in.i64);
    return in;
};

} // namespace runtime
