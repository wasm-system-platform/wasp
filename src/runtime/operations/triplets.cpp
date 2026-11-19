#include "runtime/operations/triplets.hpp"
#include "runtime/instance.hpp"

namespace runtime {

Continuation I32LocalSetConstBranch::action(Instance& instance) {
    Context& ctxt = instance.getActiveContext();
    ctxt.setLocal(local_idx_, immediate_);
    return next_;
}

} // namespace runtime
