#pragma once

#include "runtime/instance.hpp"
#include "runtime/operations/operations_complex.hpp"

namespace runtime {

inline void I32LocalAddConst::impl(I32LocalAddConst& self, Instance& instance) {
    Context& ctxt = instance.getActiveContext();
    Value& value = ctxt.getLocal(self.idx_);
    value += self.immediate_;
}

inline void I32LocalAddConstSet::impl(I32LocalAddConstSet& self,
                                      Instance& instance) {
    Context& ctxt = instance.getActiveContext();
    Value& value = ctxt.getLocal(self.source_idx_);
    ctxt.setLocal(self.target_idx_, value + self.immediate_);
}

inline Value I32LocalAddConstTee::impl(I32LocalAddConstTee& self,
                                       Instance& instance) {
    Context& ctxt = instance.getActiveContext();
    Value& value = ctxt.getLocal(self.idx_);
    value += self.immediate_;
    return value;
}

inline Value I32LocalAddConstTeeTo::impl(I32LocalAddConstTeeTo& self,
                                         Instance& instance) {
    Context& ctxt = instance.getActiveContext();
    Value value = ctxt.getLocal(self.source_idx_);
    value += self.immediate_;
    ctxt.setLocal(self.target_idx_, value);
    return value;
}

inline void I32LocalSubConst::impl(I32LocalSubConst& self, Instance& instance) {
    Context& ctxt = instance.getActiveContext();
    Value& value = ctxt.getLocal(self.idx_);
    value -= self.immediate_;
}

inline void I32LocalSubConstSet::impl(I32LocalSubConstSet& self,
                                      Instance& instance) {
    Context& ctxt = instance.getActiveContext();
    Value& value = ctxt.getLocal(self.source_idx_);
    ctxt.setLocal(self.target_idx_, value - self.immediate_);
}

inline Value I32LocalSubConstTee::impl(I32LocalSubConstTee& self,
                                       Instance& instance) {
    Context& ctxt = instance.getActiveContext();
    Value& value = ctxt.getLocal(self.idx_);
    value -= self.immediate_;
    return value;
}

inline Value I32LocalSubConstTeeTo::impl(I32LocalSubConstTeeTo& self,
                                         Instance& instance) {
    Context& ctxt = instance.getActiveContext();
    Value value = ctxt.getLocal(self.source_idx_);
    value -= self.immediate_;
    ctxt.setLocal(self.target_idx_, value);
    return value;
}

} // namespace runtime
