#include "runtime/operations/operations_complex.hpp"
#include "runtime/instance.hpp"
#include "runtime/operations/operations.hpp"

namespace runtime {

Continuation LocalAddConstCall::action(Instance& instance) {
    Context& ctxt = instance.getActiveContext();

    Value value = ctxt.getLocal(local_idx_);
    value += immediate_;

    ctxt.push(value);
    ctxt.getEpilogues().push(next_);

    Function& func = instance.getGlobalState().getFunction(func_idx_);
    return func.enterFrame(ctxt);
}

Continuation LocalSubConstCall::action(Instance& instance) {
    Context& ctxt = instance.getActiveContext();

    Value value = ctxt.getLocal(local_idx_);
    value -= immediate_;

    ctxt.push(value);
    ctxt.getEpilogues().push(next_);

    Function& func = instance.getGlobalState().getFunction(func_idx_);
    return func.enterFrame(ctxt);
}

} // namespace runtime
