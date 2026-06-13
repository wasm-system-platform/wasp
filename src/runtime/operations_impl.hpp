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
    TRACE_VERBOSE("[{:3}] {}: br_if --> {}: (cond={}) -> ()",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      br_if.addr_),
                  br_if.target_->getFormattedAddress(instance), cond.i32);

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
                  lhs.i32, rhs.i32, out.i32);
    return out;
}

inline Value I32Sub::impl(I32Sub& i32_sub, Instance& instance, Value lhs,
                          Value rhs) {
    Value out = lhs.i32 - rhs.i32;
    TRACE_VERBOSE("[{:3}] {}: i32.sub {}: ({}, {}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_sub.addr_),
                  lhs.i32, rhs.i32, out .32);
    return out;
}

inline Value F64Neg::impl(F64Neg& f64_neg, Instance& instance, Value in) {
    Value out = -in.f64;
    TRACE_VERBOSE("[{:3}] {}: f64.neg {}: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      f64_neg.addr_),
                  in.f64, out.f64);
    return out;
}

inline Value F64Add::impl(F64Add& f64_add, Instance& instance, Value lhs,
                          Value rhs) {
    Value out = lhs.f64 + rhs.f64;
    TRACE_VERBOSE("[{:3}] {}: f64.add {}: ({}, {}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      f64_add.addr_),
                  lhs.f64, rhs.f64, out.f64);
    return out;
}

inline Value F64Sub::impl(F64Sub& f64_sub, Instance& instance, Value lhs,
                          Value rhs) {
    Value out = lhs.f64 - rhs.f64;
    TRACE_VERBOSE("[{:3}] {}: f64.sub {}: ({}, {}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      f64_sub.addr_),
                  lhs.f64, rhs.f64, out.f64);
    return out;
}

inline Value
F32ConvertI32Unsigned::impl(F32ConvertI32Unsigned& f32_convert_i32_u,
                            Instance& instance, Value in) {
    Value out = static_cast<float>(static_cast<uint32_t>(in.i32));
    TRACE_VERBOSE("[{:3}] {}: f32.convert_i32_u: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      f32_convert_i32_u.addr_),
                  in.i32, out.f32);
    return out;
}

inline Value
F64ConvertI32Unsigned::impl(F64ConvertI32Unsigned& f64_convert_i32_u,
                            Instance& instance, Value in) {
    Value out = static_cast<double>(static_cast<uint32_t>(in.i32));
    TRACE_VERBOSE("[{:3}] {}: f64.convert_i32_u: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      f64_convert_i32_u.addr_),
                  in.i32, out.f64);
    return out;
}

inline Value F64ConvertI64Signed::impl(F64ConvertI64Signed& f64_convert_i64_s,
                                       Instance& instance, Value in) {
    Value out = static_cast<double>(in.i64);
    TRACE_VERBOSE("[{:3}] {}: f64.convert_i64_s: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      f64_convert_i64_s.addr_),
                  in.i64, out.f64);
    return out;
}

inline Value
F64ConvertI64Unsigned::impl(F64ConvertI64Unsigned& f64_convert_i64_u,
                            Instance& instance, Value in) {
    Value out = static_cast<double>(static_cast<uint64_t>(in.i64));
    TRACE_VERBOSE("[{:3}] {}: f64.convert_i64_u: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      f64_convert_i64_u.addr_),
                  in.i64, out.f64);
    return out;
}

inline Value I32TruncateSaturateF64Signed::impl(
    I32TruncateSaturateF64Signed& i32_trunc_sat_f64_s, Instance& instance,
    Value in) {
    const double x = in.f64;

    int32_t result;
    if (std::isnan(x)) {
        result = 0;
    } else if (x <= static_cast<double>(std::numeric_limits<int32_t>::min())) {
        result = std::numeric_limits<int32_t>::min();
    } else if (x >=
               static_cast<double>(std::numeric_limits<int32_t>::max()) + 1.0) {
        result = std::numeric_limits<int32_t>::max();
    } else {
        result = static_cast<int32_t>(x);
    }

    Value out = result;
    TRACE_VERBOSE("[{:3}] {}: i32.trunc_sat_f64_s: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_trunc_sat_f64_s.addr_),
                  in.f64, out.i32);
    return out;
}

inline Value I32TruncateSaturateF64Unsigned::impl(
    I32TruncateSaturateF64Unsigned& i32_trunc_sat_f64_u, Instance& instance,
    Value in) {
    const double x = in.f64;

    uint32_t result;
    if (std::isnan(x) || x <= 0.0) {
        result = 0;
    } else if (x >= static_cast<double>(std::numeric_limits<uint32_t>::max()) +
                        1.0) {
        result = std::numeric_limits<uint32_t>::max();
    } else {
        result = static_cast<uint32_t>(x);
    }

    Value out = static_cast<int32_t>(result);
    TRACE_VERBOSE("[{:3}] {}: i32.trunc_sat_f64_u: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_trunc_sat_f64_u.addr_),
                  in.f64, static_cast<uint32_t>(out.i32));
    return out;
}

inline Value I64TruncateSaturateF64Signed::impl(
    I64TruncateSaturateF64Signed& i64_trunc_sat_f64_s, Instance& instance,
    Value in) {
    const double x = in.f64;

    int64_t result;
    if (std::isnan(x)) {
        result = 0;
    } else if (x <= -9223372036854775808.0) {
        result = std::numeric_limits<int64_t>::min();
    } else if (x >= 9223372036854775808.0) {
        result = std::numeric_limits<int64_t>::max();
    } else {
        result = static_cast<int64_t>(x);
    }

    Value out = result;
    TRACE_VERBOSE("[{:3}] {}: i64.trunc_sat_f64_s: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i64_trunc_sat_f64_s.addr_),
                  in.f64, out.i64);
    return out;
}

} // namespace runtime
