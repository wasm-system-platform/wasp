#pragma once

#include "runtime/operations/operations_impl.hpp"
#include "runtime/operations/operations_numeric.hpp"

namespace runtime {

inline Value I32Const::impl(I32Const& i32_const, Instance& instance) {
    (void)instance;

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
    (void)i32_add;
    (void)instance;

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
    (void)i32_sub;
    (void)instance;

    Value out = lhs.i32 - rhs.i32;
    TRACE_VERBOSE("[{:3}] {}: i32.sub {}: ({}, {}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_sub.addr_),
                  lhs.i32, rhs.i32, out.32);
    return out;
}

inline Value F64Neg::impl(F64Neg& f64_neg, Instance& instance, Value in) {
    (void)f64_neg;
    (void)instance;

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
    (void)f64_add;
    (void)instance;

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
    (void)f64_sub;
    (void)instance;

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
    (void)f32_convert_i32_u;
    (void)instance;

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
    (void)f64_convert_i32_u;
    (void)instance;

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
    (void)f64_convert_i64_s;
    (void)instance;

    Value out = static_cast<double>(in.i64);
    TRACE_VERBOSE("[{:3}] {}: f64.convert_i64_s: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      f64_convert_i64_s.addr_),
                  in.i64, out.f64);
    return out;
}

inline Value I32ReinterpretF32::impl(I32ReinterpretF32& i32_reinterpret_f32,
                                     Instance& instance, Value in) {
    (void)i32_reinterpret_f32;
    (void)instance;

    Value out = static_cast<int32_t>(in.f32);
    TRACE_VERBOSE("[{:3}] {}: i32.reinterpret_f32: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_reinterpret_f32.addr_),
                  in.f32, out.i32);
    return out;
}

inline Value I64ReinterpretF64::impl(I64ReinterpretF64& i64_reinterpret_f64,
                                     Instance& instance, Value in) {
    (void)i64_reinterpret_f64;
    (void)instance;

    Value out = static_cast<int64_t>(in.f64);
    TRACE_VERBOSE("[{:3}] {}: i64.reinterpret_f64: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_reinterpret_f32.addr_),
                  in.f64, out.i64);
    return out;
}

inline Value F32ReinterpretI32::impl(F32ReinterpretI32& f32_reinterpret_i32,
                                     Instance& instance, Value in) {
    (void)f32_reinterpret_i32;
    (void)instance;

    Value out = static_cast<float>(in.i32);
    TRACE_VERBOSE("[{:3}] {}: f32.reinterpret_i32: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      i32_reinterpret_f32.addr_),
                  in.i32, out.f32);
    return out;
}

inline Value
F64ConvertI64Unsigned::impl(F64ConvertI64Unsigned& f64_convert_i64_u,
                            Instance& instance, Value in) {
    (void)f64_convert_i64_u;
    (void)instance;

    Value out = static_cast<double>(static_cast<uint64_t>(in.i64));
    TRACE_VERBOSE("[{:3}] {}: f64.convert_i64_u: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      f64_convert_i64_u.addr_),
                  in.i64, out.f64);
    return out;
}

inline Value F64ReinterpretI64::impl(F64ReinterpretI64& f64_reinterpret_i64,
                                     Instance& instance, Value in) {
    (void)f64_reinterpret_i64;
    (void)instance;

    Value out = static_cast<double>(in.i64);
    TRACE_VERBOSE("[{:3}] {}: f64.reinterpret_i64: ({}) -> ({})",
                  instance.getActiveContext().getEpilogues().size(),
                  instance.getGlobalState().getDebugInfo().getFormattedLocation(
                      f64_reinterpret_i64.addr_),
                  in.i64, out.f64);
    return out;
}

inline Value I32TruncateSaturateF64Signed::impl(
    I32TruncateSaturateF64Signed& i32_trunc_sat_f64_s, Instance& instance,
    Value in) {
    (void)i32_trunc_sat_f64_s;
    (void)instance;

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
    (void)i32_trunc_sat_f64_u;
    (void)instance;

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
    (void)i64_trunc_sat_f64_s;
    (void)instance;

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
