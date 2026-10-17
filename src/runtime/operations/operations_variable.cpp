#include "runtime/instance.hpp"
#include "runtime/operations/operations_variable.hpp"

namespace runtime {

GlobalGet::GlobalGet(const grammar::GlobalGet& global_get)
    : global_idx_(global_get.getGlobalIdx()) {}

Continuation GlobalGet::action(Instance& instance) {
    Value i = instance.getGlobalState().getGlobal(global_idx_);
    instance.getActiveContext().push(i);
    TRACE_VERBOSE("global.get {}: () -> ({})", global_idx_, i.i64);
    return next_.get();
}

GlobalSet::GlobalSet(const grammar::GlobalSet& global_set)
    : global_idx_(global_set.getGlobalIdx()) {}

Continuation GlobalSet::action(Instance& instance) {
    Value i = instance.getActiveContext().pop();
    instance.getGlobalState().setGlobal(global_idx_, i);
    TRACE_VERBOSE("global.set {}: ({}) -> ()", global_idx_, i.i64);
    return next_.get();
}

}
