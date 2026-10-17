#pragma once

#include "runtime/operations/operations.hpp"

namespace runtime {

class I32Const : public GenericOperation<I32Const> {
public:
    using InType = std::tuple<>;
    using OutType = Value;

    static OutType impl(I32Const& i32_const, Instance& instance);

    I32Const(const grammar::I32Const& i32_const)
        : GenericOperation<I32Const>(i32_const.getAddress()),
          i_(i32_const.getVal()) {}

    Expected<Continuation> eval(Context& context) const override;

private:
    int32_t i_;
};

class I64Const : public OperationBase {
public:
    I64Const(const grammar::I64Const& i64_const);

    Continuation action(Instance& instance) override;

private:
    int64_t i_;
};

class F32Const : public OperationBase {
public:
    F32Const(const grammar::F32Const& f32_const);

    Continuation action(Instance& instance) override;

private:
    float f_;
};

class F64Const : public OperationBase {
public:
    F64Const(const grammar::F64Const& f64_const);

    Continuation action(Instance& instance) override;

private:
    double f_;
};

class I32EqualZero : public TaggedOperation<I32EqualZero> {
public:
    Continuation action(Instance& instance) override;
};

class I32Equal : public TaggedOperation<I32Equal> {
public:
    Continuation action(Instance& instance) override;
};

class I32NotEqual : public TaggedOperation<I32NotEqual> {
public:
    Continuation action(Instance& instance) override;
};

class I32LessThanSigned : public TaggedOperation<I32LessThanSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32LessThanUnsigned : public TaggedOperation<I32LessThanUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32GreaterThanSigned : public TaggedOperation<I32GreaterThanSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32GreaterThanUnsigned : public TaggedOperation<I32GreaterThanUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32LessEqualSigned : public TaggedOperation<I32LessEqualSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32LessEqualUnsigned : public TaggedOperation<I32LessEqualUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32GreaterEqualSigned : public TaggedOperation<I32GreaterEqualSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32GreaterEqualUnsigned
    : public TaggedOperation<I32GreaterEqualUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64EqualZero : public TaggedOperation<I64EqualZero> {
public:
    Continuation action(Instance& instance) override;
};

class I64Equal : public TaggedOperation<I64Equal> {
public:
    Continuation action(Instance& instance) override;
};

class I64NotEqual : public TaggedOperation<I64NotEqual> {
public:
    Continuation action(Instance& instance) override;
};

class I64LessThanSigned : public TaggedOperation<I64LessThanSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64LessThanUnsigned : public TaggedOperation<I64LessThanUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64GreaterThanSigned : public TaggedOperation<I64GreaterThanSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64GreaterThanUnsigned : public TaggedOperation<I64GreaterThanUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64LessEqualSigned : public TaggedOperation<I64LessEqualSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64LessEqualUnsigned : public TaggedOperation<I64LessEqualUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64GreaterEqualSigned : public TaggedOperation<I64GreaterEqualSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64GreaterEqualUnsigned
    : public TaggedOperation<I64GreaterEqualUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class F32Equal : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F32NotEqual : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F32LessThan : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F32GreaterThan : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F32LessEqual : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F32GreaterEqual : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64Equal : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64NotEqual : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64LessThan : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64GreaterThan : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64LessEqual : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64GreaterEqual : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32CountLeadingZeros : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32CountTrailingZeros : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32PopCount : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32Add : public GenericOperation<I32Add> {
public:
    using InType = std::tuple<Value, Value>;
    using OutType = Value;

    static OutType impl(I32Add& i32_add, Instance& instance, Value lhs,
                        Value rhs);

    I32Add(const grammar::I32Add& i32_add)
        : GenericOperation<I32Add>(i32_add.getAddress()) {}
};

class I32Sub : public GenericOperation<I32Sub> {
public:
    using InType = std::tuple<Value, Value>;
    using OutType = Value;

    static OutType impl(I32Sub& i32_sub, Instance& instance, Value lhs,
                        Value rhs);

    I32Sub(const grammar::I32Sub& i32_sub)
        : GenericOperation<I32Sub>(i32_sub.getAddress()) {}
};

class I32Mul : public TaggedOperation<I32Mul> {
public:
    Continuation action(Instance& instance) override;
};

class I32DivideSigned : public TaggedOperation<I32DivideSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32DivideUnsigned : public TaggedOperation<I32DivideUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32RemainderSigned : public TaggedOperation<I32RemainderSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32RemainderUnsigned : public TaggedOperation<I32RemainderUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32And : public TaggedOperation<I32And> {
public:
    Continuation action(Instance& instance) override;
};

class I32Or : public TaggedOperation<I32Or> {
public:
    Continuation action(Instance& instance) override;
};

class I32Xor : public TaggedOperation<I32Xor> {
public:
    Continuation action(Instance& instance) override;
};

class I32ShiftLeft : public TaggedOperation<I32ShiftLeft> {
public:
    Continuation action(Instance& instance) override;
};

class I32ShiftRightSigned : public TaggedOperation<I32ShiftRightSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32ShiftRightUnsigned : public TaggedOperation<I32ShiftRightUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I32RotateLeft : public TaggedOperation<I32RotateLeft> {
public:
    Continuation action(Instance& instance) override;
};

class I32RotateRight : public TaggedOperation<I32RotateRight> {
public:
    Continuation action(Instance& instance) override;
};

class I64CountLeadingZeros : public TaggedOperation<I64CountLeadingZeros> {
public:
    Continuation action(Instance& instance) override;
};

class I64CountTrailingZeros : public TaggedOperation<I64CountTrailingZeros> {
public:
    Continuation action(Instance& instance) override;
};

class I64PopCount : public TaggedOperation<I64PopCount> {
public:
    Continuation action(Instance& instance) override;
};

class I64Add : public TaggedOperation<I64Add> {
public:
    Continuation action(Instance& instance) override;
};

class I64Sub : public TaggedOperation<I64Sub> {
public:
    Continuation action(Instance& instance) override;
};

class I64Mul : public TaggedOperation<I64Mul> {
public:
    Continuation action(Instance& instance) override;
};

class I64DivideSigned : public TaggedOperation<I64DivideSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64DivideUnsigned : public TaggedOperation<I64DivideUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64RemainderSigned : public TaggedOperation<I64RemainderSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64RemainderUnsigned : public TaggedOperation<I64RemainderUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64And : public TaggedOperation<I64And> {
public:
    Continuation action(Instance& instance) override;
};

class I64Or : public TaggedOperation<I64Or> {
public:
    Continuation action(Instance& instance) override;
};

class I64Xor : public TaggedOperation<I64Xor> {
public:
    Continuation action(Instance& instance) override;
};

class I64ShiftLeft : public TaggedOperation<I64ShiftLeft> {
public:
    Continuation action(Instance& instance) override;
};

class I64ShiftRightSigned : public TaggedOperation<I64ShiftRightSigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64ShiftRightUnsigned : public TaggedOperation<I64ShiftRightUnsigned> {
public:
    Continuation action(Instance& instance) override;
};

class I64RotateLeft : public TaggedOperation<I64RotateLeft> {
public:
    Continuation action(Instance& instance) override;
};

class I64RotateRight : public TaggedOperation<I64RotateRight> {
public:
    Continuation action(Instance& instance) override;
};

class F32Mul : public TaggedOperation<F32Mul> {
public:
    Continuation action(Instance& instance) override;
};

class F32Div : public TaggedOperation<F32Div> {
public:
    Continuation action(Instance& instance) override;
};

class F64Neg : public GenericOperation<F64Neg> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(F64Neg& f64_neg, Instance& instance, Value in);

    F64Neg(const grammar::F64Neg& f64_neg)
        : GenericOperation<F64Neg>(f64_neg.getAddress()) {}
};

class F64Add : public GenericOperation<F64Add> {
public:
    using InType = Values<2>;
    using OutType = Values<1>;

    static OutType impl(F64Add& f64_add, Instance& instance, Value lhs,
                        Value rhs);

    F64Add(const grammar::F64Add& f64_add)
        : GenericOperation<F64Add>(f64_add.getAddress()) {}
};

class F64Sub : public GenericOperation<F64Sub> {
public:
    using InType = Values<2>;
    using OutType = Values<1>;

    static OutType impl(F64Sub& f64_sub, Instance& instance, Value lhs,
                        Value rhs);

    F64Sub(const grammar::F64Sub& f64_sub)
        : GenericOperation<F64Sub>(f64_sub.getAddress()) {}
};

class F64Mul : public TaggedOperation<F64Mul> {
public:
    Continuation action(Instance& instance) override;
};

class F64Div : public TaggedOperation<F64Div> {
public:
    Continuation action(Instance& instance) override;
};

class F64CopySign : public TaggedOperation<F64CopySign> {
public:
    Continuation action(Instance& instance) override;
};

class I32WrapI64 : public TaggedOperation<I32WrapI64> {
public:
    Continuation action(Instance& instance) override;
};

class I64ExtendI32Signed : public TaggedOperation<I64ExtendI32Signed> {
public:
    Continuation action(Instance& instance) override;
};

class I64ExtendI32Unsigned : public TaggedOperation<I64ExtendI32Unsigned> {
public:
    Continuation action(Instance& instance) override;
};

class F32ConvertI32Signed : public TaggedOperation<F32ConvertI32Signed> {
public:
    Continuation action(Instance& instance) override;
};

class F32ConvertI32Unsigned : public GenericOperation<F32ConvertI32Unsigned> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(F32ConvertI32Unsigned& f32_convert_i32_u,
                        Instance& instance, Value in);
};

class F32DemoteF64 : public TaggedOperation<F32DemoteF64> {
public:
    Continuation action(Instance& instance) override;
};

class F64ConvertI32Signed : public TaggedOperation<F64ConvertI32Signed> {
public:
    Continuation action(Instance& instance) override;
};

class F64ConvertI32Unsigned : public GenericOperation<F64ConvertI32Unsigned> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(F64ConvertI32Unsigned& f64_convert_i32_u,
                        Instance& instance, Value in);
};

class F64ConvertI64Signed : public GenericOperation<F64ConvertI64Signed> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(F64ConvertI64Signed& f64_convert_i64_s,
                        Instance& instance, Value in);
};

class F64ConvertI64Unsigned : public GenericOperation<F64ConvertI64Unsigned> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(F64ConvertI64Unsigned& f64_convert_i64_u,
                        Instance& instance, Value in);
};

class F64PromoteF32 : public TaggedOperation<F64PromoteF32> {
public:
    Continuation action(Instance& instance) override;
};

class I32ReinterpretF32 : public GenericOperation<I32ReinterpretF32> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(I32ReinterpretF32& i32_reinterpret_f32,
                        Instance& instance, Value in);
};

class I64ReinterpretF64 : public TaggedOperation<I64ReinterpretF64> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(I64ReinterpretF64& i64_reinterpret_f64,
                        Instance& instance, Value in);
};

class F32ReinterpretI32 : public GenericOperation<F32ReinterpretI32> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(F32ReinterpretI32& f32_reinterpret_i32,
                        Instance& instance, Value in);
};

class F64ReinterpretI64 : public GenericOperation<F64ReinterpretI64> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(F64ReinterpretI64& f64_reinterpret_i64,
                        Instance& instance, Value in);
};

class I32Extend8Signed : public TaggedOperation<I32Extend8Signed> {
public:
    Continuation action(Instance& instance) override;
};

class I32Extend16Signed : public TaggedOperation<I32Extend16Signed> {
public:
    Continuation action(Instance& instance) override;
};

class I64Extend8Signed : public TaggedOperation<I64Extend8Signed> {
public:
    Continuation action(Instance& instance) override;
};

class I64Extend16Signed : public TaggedOperation<I64Extend16Signed> {
public:
    Continuation action(Instance& instance) override;
};

class I64Extend32Signed : public TaggedOperation<I64Extend32Signed> {
public:
    Continuation action(Instance& instance) override;
};

class I32TruncateSaturateF64Signed
    : public GenericOperation<I32TruncateSaturateF64Signed> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(I32TruncateSaturateF64Signed& i32_trunc_sat_f64_s,
                        Instance& instance, Value in);

    I32TruncateSaturateF64Signed(
        const grammar::I32TruncateSaturateF64Signed& i32_trunc_sat_f64_s)
        : GenericOperation<I32TruncateSaturateF64Signed>(
              i32_trunc_sat_f64_s.getAddress()) {}
};

class I32TruncateSaturateF64Unsigned
    : public GenericOperation<I32TruncateSaturateF64Unsigned> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(I32TruncateSaturateF64Unsigned& i32_trunc_sat_f64_u,
                        Instance& instance, Value in);

    I32TruncateSaturateF64Unsigned(
        const grammar::I32TruncateSaturateF64Unsigned& i32_trunc_sat_f64_u)
        : GenericOperation<I32TruncateSaturateF64Unsigned>(
              i32_trunc_sat_f64_u.getAddress()) {}
};

class I64TruncateSaturateF64Signed
    : public GenericOperation<I64TruncateSaturateF64Signed> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(I64TruncateSaturateF64Signed& i64_trunc_sat_f64_s,
                        Instance& instance, Value in);

    I64TruncateSaturateF64Signed(
        const grammar::I64TruncateSaturateF64Signed& i64_trunc_sat_f64_s)
        : GenericOperation<I64TruncateSaturateF64Signed>(
              i64_trunc_sat_f64_s.getAddress()) {}
};

}
