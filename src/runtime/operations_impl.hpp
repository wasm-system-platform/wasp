#include "runtime/instance.hpp"

namespace runtime {

/*************************/
/* Variable Instructions */
/*************************/

inline Value LocalGet::impl(LocalGet& local_get, Instance& instance) {
    Value local = instance.getActiveContext().getLocal(local_get.local_idx_);
    TRACE_VERBOSE("[{:3}] {}: local.get {}: () -> (i32={}, i64={})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      local_get.addr_),
                  local_get.local_idx_, local.i32, local.i64);
    return local;
}

inline void LocalSet::impl(LocalSet& local_set, Instance& instance,
                           Value local) {
    instance.getActiveContext().setLocal(local_set.local_idx_, local);
    TRACE_VERBOSE("[{:3}] {}: local.set {}: (i32={}, i64={}) -> ()",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      local_set.addr_),
                  local_set.local_idx_, value.i32, value.i64);
};

// inline Value LocalTee::impl(LocalTee& local_tee, Instance& instance, Value
// value) {
//     instance.getActiveContext().setLocal(local_tee.local_idx_, value);
//     TRACE_VERBOSE(
//         "[{:3}] {}: local.tee {}: ({}) -> ({})",
//         instance.getActiveContext().getEpilogues().size(),
//         instance.getGlobalState().getDebugInfo().getFormattedLocation(local_tee.addr_),
//         local_tee.local_idx_, value.i64, value.i64);
//     return value;
// };

/************************/
/* Numeric Instructions */
/************************/

inline Value I32Const::impl(I32Const& i32_const, Instance& instance) {
    TRACE_VERBOSE("[{:3}] {}: i32.const {}: () -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_const.addr_),
                  i32_const.i_, i32_const.i_);
    return Value(i32_const.i_);
}

} // namespace runtime
