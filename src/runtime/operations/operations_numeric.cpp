#include "runtime/context.hpp"
#include "runtime/instance.hpp"
#include "runtime/operations/operations_numeric.hpp"
#include "runtime/operations/operations_numeric_impl.hpp"

namespace runtime {

/************************/
/* Numeric Instructions */
/************************/

Expected<Continuation> I32Const::eval(Context& context) const {
    context.pushI32(i_);
    return next_.get();
}

I64Const::I64Const(const grammar::I64Const& i64_const)
    : i_(i64_const.getVal()) {}

Continuation I64Const::action(Instance& instance) {
    instance.getActiveContext().pushI64(i_);
    return next_.get();
}

F32Const::F32Const(const grammar::F32Const& f32_const)
    : f_(f32_const.getVal()) {}

Continuation F32Const::action(Instance& instance) {
    instance.getActiveContext().push(f_);
    return next_.get();
}

F64Const::F64Const(const grammar::F64Const& f64_const)
    : f_(f64_const.getVal()) {}

Continuation F64Const::action(Instance& instance) {
    instance.getActiveContext().push(f_);
    return next_.get();
}

Continuation I32EqualZero::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.pop().i32;
    int32_t result = val == 0 ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.eqz: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I32Equal::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    int32_t left = context.pop().i32;
    int32_t result = (left == right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.eq: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32NotEqual::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    int32_t left = context.pop().i32;
    int32_t result = (left != right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.ne: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32LessThanSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    int32_t left = context.pop().i32;
    int32_t result = (left < right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.lt_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32LessThanUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.pop().i32);
    uint32_t left = static_cast<uint32_t>(context.pop().i32);
    int32_t result = (left < right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.lt_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32GreaterThanSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    int32_t left = context.pop().i32;
    int32_t result = (left > right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.gt_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32GreaterThanUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.pop().i32);
    uint32_t left = static_cast<uint32_t>(context.pop().i32);
    int32_t result = (left > right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.gt_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32LessEqualSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    int32_t left = context.pop().i32;
    int32_t result = (left <= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.ge_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32LessEqualUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.pop().i32);
    uint32_t left = static_cast<uint32_t>(context.pop().i32);
    int32_t result = (left <= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.ge_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32GreaterEqualSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    int32_t left = context.pop().i32;
    int32_t result = (left >= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.ge_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32GreaterEqualUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.pop().i32);
    uint32_t left = static_cast<uint32_t>(context.pop().i32);
    int32_t result = (left >= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.ge_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64EqualZero::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t val = context.pop().i64;
    int32_t result = val == 0 ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.eqz: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I64Equal::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int32_t result = (left == right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.eq: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64NotEqual::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int32_t result = (left != right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.ne: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64LessThanSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int32_t result = (left < right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.lt_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64LessThanUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t right = static_cast<uint64_t>(context.pop().i64);
    uint64_t left = static_cast<uint64_t>(context.pop().i64);
    int32_t result = (left < right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.lt_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64GreaterThanSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int32_t result = (left > right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.gt_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64GreaterThanUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t right = static_cast<uint64_t>(context.pop().i64);
    uint64_t left = static_cast<uint64_t>(context.pop().i64);
    int32_t result = (left > right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.gt_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64LessEqualSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int32_t result = (left <= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.le_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64LessEqualUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t right = static_cast<uint64_t>(context.pop().i64);
    uint64_t left = static_cast<uint64_t>(context.pop().i64);
    int32_t result = (left <= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.le_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64GreaterEqualSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int32_t result = (left >= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.ge_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64GreaterEqualUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t right = static_cast<uint64_t>(context.pop().i64);
    uint64_t left = static_cast<uint64_t>(context.pop().i64);
    int32_t result = (left >= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.ge_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F32Equal::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float right = context.pop().f32;
    float left = context.pop().f32;
    int32_t result = (left == right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f32.eq: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F32NotEqual::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float right = context.pop().f32;
    float left = context.pop().f32;
    int32_t result = (left != right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f32.ne: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F32LessThan::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float right = context.pop().f32;
    float left = context.pop().f32;
    int32_t result = (left < right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f32.lt: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F32GreaterThan::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float right = context.pop().f32;
    float left = context.pop().f32;
    int32_t result = (left > right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f32.gt: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F32LessEqual::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float right = context.pop().f32;
    float left = context.pop().f32;
    int32_t result = (left <= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f32.le: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F32GreaterEqual::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float right = context.pop().f32;
    float left = context.pop().f32;
    int32_t result = (left >= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f32.ge: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F64Equal::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double right = context.pop().f64;
    double left = context.pop().f64;
    int32_t result = (left == right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f64.eq: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F64NotEqual::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double right = context.pop().f64;
    double left = context.pop().f64;
    int32_t result = (left != right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f64.ne: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F64LessThan::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double right = context.pop().f64;
    double left = context.pop().f64;
    int32_t result = (left < right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f64.lt: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F64GreaterThan::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double right = context.pop().f64;
    double left = context.pop().f64;
    int32_t result = (left > right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f64.gt: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F64LessEqual::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double right = context.pop().f64;
    double left = context.pop().f64;
    int32_t result = (left <= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f64.le: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F64GreaterEqual::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double right = context.pop().f64;
    double left = context.pop().f64;
    int32_t result = (left >= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("f64.ge: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32CountLeadingZeros::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t val = static_cast<uint32_t>(context.pop().i32);
    int32_t result = std::countl_zero(val);
    context.pushI32(result);

    TRACE_VERBOSE("i32.clz: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I32CountTrailingZeros::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t val = static_cast<uint32_t>(context.pop().i32);
    int32_t result = std::countr_zero(val);
    context.pushI32(result);

    TRACE_VERBOSE("i32.ctz: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I32PopCount::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t val = static_cast<uint32_t>(context.pop().i32);
    int32_t result = std::popcount(val);
    context.pushI32(result);

    TRACE_VERBOSE("i32.popcnt: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I32Mul::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    int32_t left = context.pop().i32;
    int32_t result = left * right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.mul: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32DivideSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    if (right == 0)
        return trap(instance, "i32.div_s: division by zero", addr_);

    int32_t left = context.pop().i32;
    if (left == std::numeric_limits<int32_t>::min() && right == -1)
        return trap(instance, "i32.div_s: integer overflow (INT32_MIN / -1)",
                    addr_);

    int32_t result = left / right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.div_s: (left={}, right={}) -> ({})", left, right,
                  result);
    return next_.get();
}

Continuation I32DivideUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.pop().i32);
    if (right == 0)
        return trap(instance, "i32.div_u: division by zero", addr_);

    uint32_t left = static_cast<uint32_t>(context.pop().i32);
    uint32_t result = left / right;
    context.pushI32(static_cast<int32_t>(result));

    TRACE_VERBOSE("i32.div_u: (left={}, right={}) -> ({})", left, right,
                  result);
    return next_.get();
}

Continuation I32RemainderSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    if (right == 0)
        return trap(instance, "i32.div_s: division by zero", addr_);

    int32_t left = context.pop().i32;
    int32_t result = left % right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.rem_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32RemainderUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.pop().i32);
    if (right == 0)
        return trap(instance, "i32.div_u: division by zero", addr_);

    uint32_t left = static_cast<uint32_t>(context.pop().i32);
    uint32_t result = left % right;
    context.pushI32(static_cast<int32_t>(result));

    TRACE_VERBOSE("i32.rem_u: (left={}, right={}) -> ({})", left, right,
                  result);
    return next_.get();
}

Continuation I32And::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    int32_t left = context.pop().i32;
    int32_t result = left & right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.and: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32Or::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    int32_t left = context.pop().i32;
    int32_t result = left | right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.or: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32Xor::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32;
    int32_t left = context.pop().i32;
    int32_t result = left ^ right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.xor: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32ShiftLeft::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32 & 31;
    int32_t left = context.pop().i32;
    int32_t result = left << right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.shl: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32ShiftRightSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32 & 31;
    int32_t left = context.pop().i32;
    int32_t result = left >> right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.shr_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32ShiftRightUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.pop().i32 & 31;
    uint32_t left = static_cast<uint32_t>(context.pop().i32);
    uint32_t result = left >> right;
    context.pushI32(static_cast<int32_t>(result));

    TRACE_VERBOSE("i32.shr_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32RotateLeft::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t shift = context.pop().i32 & 31;
    uint32_t value = static_cast<uint32_t>(context.pop().i32);
    uint32_t result = std::rotl(value, static_cast<int>(shift));
    context.pushI32(static_cast<int32_t>(result));

    TRACE_VERBOSE("i32.rotl: ({}, {}) -> ({})", value, shift, result);

    return next_.get();
}

Continuation I32RotateRight::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t shift = context.pop().i32 & 31;
    uint32_t value = static_cast<uint32_t>(context.pop().i32);
    uint32_t result = std::rotr(value, static_cast<int>(shift));
    context.pushI32(static_cast<int32_t>(result));

    TRACE_VERBOSE("i32.rotl: ({}, {}) -> ({})", value, shift, result);

    return next_.get();
}

Continuation I64CountLeadingZeros::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t val = static_cast<uint64_t>(context.pop().i64);
    int64_t result;

    if (val == 0)
        result = 64;
    else
        result = __builtin_clzll(val);

    context.pushI64(result);

    TRACE_VERBOSE("i64.clz: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I64CountTrailingZeros::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t val = static_cast<uint64_t>(context.pop().i64);
    int64_t result;

    if (val == 0)
        result = 64;
    else
        result = __builtin_ctzll(val);

    context.pushI64(result);

    TRACE_VERBOSE("i64.ctz: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I64PopCount::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t val = static_cast<uint64_t>(context.pop().i64);
    int64_t result = std::popcount(val);
    context.pushI64(result);

    TRACE_VERBOSE("i64.popcnt: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I64Add::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int64_t result = left + right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.add: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64Sub::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int64_t result = left - right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.sub: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64Mul::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int64_t result = left * right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.mul: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64DivideSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    if (right == 0)
        return trap(instance, "i64.div_s: division by zero", addr_);

    int64_t left = context.pop().i64;
    if (left == std::numeric_limits<int64_t>::min() && right == -1)
        return trap(instance, "i64.div_s: integer overflow (INT64_MIN / -1)",
                    addr_);

    int64_t result = left / right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.div_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64DivideUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t right = static_cast<uint64_t>(context.pop().i64);
    if (right == 0)
        return trap(instance, "i64.div_u: division by zero", addr_);

    uint64_t left = static_cast<uint64_t>(context.pop().i64);
    uint64_t result = left / right;
    context.pushI64(static_cast<int64_t>(result));

    TRACE_VERBOSE("i64.div_u: (left={}, right={}) -> ({})", left, right,
                  result);
    return next_.get();
}

Continuation I64RemainderSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    if (right == 0)
        return trap(instance, "i64.rem_s: division by zero", addr_);

    int64_t left = context.pop().i64;
    int64_t result = left % right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.rem_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64RemainderUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t right = static_cast<uint64_t>(context.pop().i64);
    if (right == 0)
        return trap(instance, "i64.rem_u: division by zero", addr_);

    uint64_t left = static_cast<uint64_t>(context.pop().i64);
    uint64_t result = left % right;
    context.pushI64(static_cast<int64_t>(result));

    TRACE_VERBOSE("i64.rem_u: (left={}, right={}) -> ({})", left, right,
                  result);
    return next_.get();
}

Continuation I64And::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int64_t result = left & right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.and: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64Or::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int64_t result = left | right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.or: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64Xor::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64;
    int64_t left = context.pop().i64;
    int64_t result = left ^ right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.xor: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64ShiftLeft::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64 & 63;
    int64_t left = context.pop().i64;
    int64_t result = left << right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.shl: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64ShiftRightSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64 & 63;
    int64_t left = context.pop().i64;
    int64_t result = left >> right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.shr_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64ShiftRightUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.pop().i64 & 63;
    uint64_t left = static_cast<uint64_t>(context.pop().i64);
    uint64_t result = left >> right;
    context.pushI64(static_cast<int64_t>(result));

    TRACE_VERBOSE("i64.shr_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64RotateLeft::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t y = static_cast<uint64_t>(context.pop().i64) & 63;
    uint64_t x = static_cast<uint64_t>(context.pop().i64);
    uint64_t result = __builtin_rotateleft64(x, y);
    context.pushI64(static_cast<int64_t>(result));

    TRACE_VERBOSE("i64.rotl: ({}, {}) -> ({})", x, y, result);
    return next_.get();
}

Continuation I64RotateRight::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t y = static_cast<uint64_t>(context.pop().i64) & 63;
    uint64_t x = static_cast<uint64_t>(context.pop().i64);
    uint64_t result = __builtin_rotateright64(x, y);
    context.pushI64(static_cast<int64_t>(result));

    TRACE_VERBOSE("i64.rotr: ({}, {}) -> ({})", x, y, result);
    return next_.get();
}

Continuation F32Mul::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float right = context.pop().f32;
    float left = context.pop().f32;
    float result = left * right;
    context.push(result);

    TRACE_VERBOSE("f32.mul: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F32Div::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float right = context.pop().f32;
    float left = context.pop().f32;
    float result = left / right;
    context.push(result);

    TRACE_VERBOSE("f32.div: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F64Mul::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double right = context.pop().f64;
    double left = context.pop().f64;
    double result = left * right;
    context.push(result);

    TRACE_VERBOSE("f64.mul: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F64Div::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double right = context.pop().f64;
    double left = context.pop().f64;
    double result = left / right;
    context.push(result);

    TRACE_VERBOSE("f64.div: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation F64CopySign::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double y = context.pop().f64;
    double x = context.pop().f64;
    double result = std::copysign(x, y);
    context.push(result);

    TRACE_VERBOSE("f64.copysign: ({}, {}) -> ({})", x, y, result);
    return next_.get();
}

Continuation I32WrapI64::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t val = context.pop().i64;
    int32_t result = static_cast<int32_t>(val);
    context.pushI32(result);

    TRACE_VERBOSE("i32.wrap_i64: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I64ExtendI32Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.pop().i32;
    context.pushI64(val);

    TRACE_VERBOSE("i64.extend_i32_s: ({}) -> ({})", val, val);
    return next_.get();
}

Continuation I64ExtendI32Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t val = static_cast<uint32_t>(context.pop().i32);
    context.pushI64(val);

    TRACE_VERBOSE("extend_i32_u: ({}) -> ({})", val, val);
    return next_.get();
}

Continuation F32ConvertI32Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.pop().i32;
    context.push(static_cast<float>(val));

    TRACE_VERBOSE("f32.convert_s_i32: ({}) -> ({})", val,
                  static_cast<float>(val));
    return next_.get();
}

Continuation F32DemoteF64::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double val = context.pop().f64;
    context.push(static_cast<float>(val));

    TRACE_VERBOSE("f32.demote_f64: ({}) -> ({})", val, static_cast<float>(val));
    return next_.get();
}

Continuation F64ConvertI32Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.pop().i32;
    context.push(static_cast<double>(val));

    TRACE_VERBOSE("f64.convert_i32_s: ({}) -> ({})", val,
                  static_cast<double>(val));
    return next_.get();
}

Continuation F64PromoteF32::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float val = context.pop().f32;
    context.push(static_cast<double>(val));

    TRACE_VERBOSE("f64.promote_f32: ({}) -> ({})", val,
                  static_cast<double>(val));
    return next_.get();
}

Continuation I32Extend8Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.pop().i32;
    int32_t result = static_cast<int8_t>(val);
    context.push(result);

    TRACE_VERBOSE("i32.extend8_s: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I32Extend16Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.pop().i32;
    int32_t result = static_cast<int16_t>(val);
    context.push(result);

    TRACE_VERBOSE("i32.extend16_s: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I64Extend8Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t val = context.pop().i64;
    int64_t result = static_cast<int8_t>(val);
    context.push(result);

    TRACE_VERBOSE("i64.extend8_s: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I64Extend16Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t val = context.pop().i64;
    int64_t result = static_cast<int16_t>(val);
    context.push(result);

    TRACE_VERBOSE("i64.extend16_s: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I64Extend32Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t val = context.pop().i64;
    int64_t result = static_cast<int32_t>(val);
    context.push(result);

    TRACE_VERBOSE("i64.extend32_s: ({}) -> ({})", val, result);
    return next_.get();
}

}
