#include "runtime/instance.hpp"

namespace runtime {

template <typename Derived>
Continuation GenericOperation<Derived>::action(Instance& instance) {
    Stack& stack = instance.getActiveContext().getStack();

    if constexpr (std::is_same_v<typename Derived::InType, void>) {
        if constexpr (std::is_same_v<typename Derived::OutType, void>) {
            Derived::impl(static_cast<Derived&>(*this), instance);
        } else {
            auto out = Derived::impl(static_cast<Derived&>(*this), instance);
            stack.push(out);
        }
    } else if constexpr (std::is_same_v<typename Derived::InType, Value>) {
        Value in = stack.pop();

        if constexpr (std::is_same_v<typename Derived::OutType, void>) {
            Derived::impl(static_cast<Derived&>(*this), instance, in);
        } else {
            auto out =
                Derived::impl(static_cast<Derived&>(*this), instance, in);
            stack.push(out);
        }
    } else {
        auto in = stack.pop<typename Derived::InType>();

        if constexpr (std::is_same_v<typename Derived::OutType, void>) {
            std::apply(
                [&](auto&&... args) {
                    Derived::impl(static_cast<Derived&>(*this), instance,
                                  std::forward<decltype(args)>(args)...);
                },
                in);
        } else {
            auto out = std::apply(
                [&](auto&&... args) {
                    return Derived::impl(static_cast<Derived&>(*this), instance,
                                         std::forward<decltype(args)>(args)...);
                },
                in);
            stack.push(out);
        }
    }

    return this->next_.get();
}

/**********************/
/* Control Operations */
/**********************/

inline Continuation IfThen::impl(IfThen& if_then, Instance& instance,
                                 Continuation next, Value cond) {
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
    TRACE("[{:3}]: br_if --> {}: (cond={}) -> ()",
          instance.getActiveContext().getEpilogues().size(),
          instance.getGlobalState().getDebugInfo().getFormattedLocation(addr_),
          target_->getFormattedAddress(instance), cond);

    if (cond.i32)
        return br_if.target_.get();

    return next;
}

/*************************/
/* Variable Instructions */
/*************************/

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

/************************/
/* Numeric Instructions */
/************************/

inline Value I32Const::impl(I32Const& i32_const, Instance& instance) {
    Value out = i32_const.i_;
    TRACE_VERBOSE("[{:3}] {}: i32.const {}: () -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_const.addr_),
                  i32_const.i_, i32_const.i_);
    return out;
}

inline Value I32Add::impl(I32Add& i32_add, Instance& instance, Value lhs,
                          Value rhs) {
    Value out = lhs.i32 + rhs.i32;
    TRACE_VERBOSE("[{:3}] {}: i32.add {}: ({}, {}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_add.addr_),
                  lhs.i32, rhs.i32, out);
    return out;
}

inline Value I32Sub::impl(I32Sub& i32_sub, Instance& instance, Value lhs,
                          Value rhs) {
    Value out = lhs.i32 - rhs.i32;
    TRACE_VERBOSE("[{:3}] {}: i32.sub {}: ({}, {}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_sub.addr_),
                  lhs.i32, rhs.i32, out);
    return out;
}

} // namespace runtime
