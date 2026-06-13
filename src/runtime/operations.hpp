#pragma once

#include <cassert>
#include <cxxabi.h>
#include <stack>

#include "grammar/sections.hpp"

namespace runtime {

struct Value {
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
    };

    Value() = default;
    Value(int32_t i32) noexcept : i32(i32) {}
    Value(int64_t i64) noexcept : i64(i64) {}
    Value(float f32) noexcept : f32(f32) {}
    Value(double f64) noexcept : f64(f64) {}
};
static_assert(std::is_trivially_copyable_v<Value>);

template <std::size_t num> struct ValueTuple {
    template <std::size_t> using AlwaysValue = Value;

    template <std::size_t... I>
    static auto
        make(std::index_sequence<I...>) -> std::tuple<AlwaysValue<I>...>;

    using type = decltype(make(std::make_index_sequence<num>{}));
};

template <> struct ValueTuple<1> {
    using type = Value;
};

template <> struct ValueTuple<0> {
    using type = void;
};

template <std::size_t num> using Values = typename ValueTuple<num>::type;

class Instance;
class Context;
class GlobalState;
class Function;

class OperationBase;
using Operation = std::shared_ptr<OperationBase>;
using Operations = std::vector<Operation>;
using Continuation = OperationBase*;

class OperationBase : public std::enable_shared_from_this<OperationBase> {
public:
    using TypeId = const void*;

    virtual ~OperationBase() = default;

    static Operation
    create(const std::vector<grammar::Instruction>& instructions,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& targets);

    virtual Continuation action(Instance&) { return next_.get(); }
    virtual Expected<Continuation> eval(Context&) const {
        return Unexpected(ERROR("evaluate called on non constant expression"));
    }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }
    template <class Derived> Derived& as() {
        return reinterpret_cast<Derived&>(*this);
    }
    template <class Derived> bool is() {
        return type() == Derived::static_type();
    }

    virtual TypeId type() const { return nullptr; }

    virtual std::string getName() { return "?"; }

    size_t getAddress() { return addr_; }
    std::string getFormattedAddress(Instance& instance) const;

protected:
    class Builder {
    public:
        Operation build();

        void addNext(Operation next);

    private:
        Operations operations_;
    };

    OperationBase() = default;
    OperationBase(size_t addr) : addr_(addr) {}

    Continuation trap(Instance& instance, std::string msg, size_t addr) const;

    Operation next_;
    size_t addr_ = UINT32_MAX;
};

template <typename Derived> class TaggedOperation : public OperationBase {
public:
    using OperationBase::OperationBase;

    static TypeId static_type() {
        static int id;
        return &id;
    }

    TypeId type() const override { return static_type(); }

    std::string getName() override {
        int status = 0;
        char* demangled = abi::__cxa_demangle(typeid(Derived).name(), nullptr,
                                              nullptr, &status);
        std::string name = (status == 0) ? demangled : typeid(Derived).name();
        std::free(demangled);

        auto pos = name.rfind("::");
        if (pos == std::string_view::npos)
            return std::string(name);

        return std::string(name.substr(pos + 2));
    }
};

template <typename Derived>
class GenericOperation : public TaggedOperation<Derived> {
public:
    using TaggedOperation<Derived>::TaggedOperation;

    Continuation action(Instance& instance) override;
};

/**********************/
/* Control Operations */
/**********************/

class Unreachable : public OperationBase {
public:
    Unreachable(const grammar::Unreachable& unreachable);

    Continuation action(Instance& instance) override;
};

class Nop : public TaggedOperation<Nop> {
public:
    Nop(const grammar::Nop& nop) : TaggedOperation<Nop>(nop.getAddress()) {}
    Nop(size_t addr) : TaggedOperation<Nop>(addr) {}

    void setAddress(size_t addr) { addr_ = addr; }
};

class Label : public TaggedOperation<Label> {
public:
    Label(size_t addr) : TaggedOperation<Label>(addr) {
        next_ = std::make_shared<Nop>(addr);
    }

    Continuation action(Instance& instance) override {
        return next_->action(instance);
    }
};

class Block : public TaggedOperation<Block> {
public:
    Block(const grammar::Block& block,
          const std::vector<FunctionType>& func_types,
          std::vector<Operation>& branch_targets);

    Continuation action(Instance& instance) override;

private:
    Operation body_;
};

class Loop : public TaggedOperation<Loop> {
public:
    Loop(const grammar::Loop& loop, const std::vector<FunctionType>& func_types,
         std::vector<Operation>& branch_targets);

    Continuation action(Instance& instance) override;

private:
    Operation body_;
};

class IfThen : public TaggedOperation<IfThen> {
public:
    static Continuation impl(IfThen& if_then, Instance& instance,
                             Continuation next, Value cond);

    IfThen(const grammar::IfElse& if_else,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& branch_targets);

    Continuation action(Instance& instance) override;

protected:
    Operation then_;
};

class IfElse : public TaggedOperation<IfElse> {
public:
    static Continuation impl(IfElse& if_else, Instance& instance, Value cond);

    IfElse(const grammar::IfElse& if_else,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& branch_targets);

    Continuation action(Instance& instance) override;

private:
    Operation then_;
    Operation else_;
};

class Branch : public TaggedOperation<Branch> {
public:
    Branch(const grammar::Branch& branch, std::vector<Operation>& targets);

    Continuation action(Instance& instance) override;

private:
    Operation target_;
};

class BranchIf : public TaggedOperation<BranchIf> {
public:
    static Continuation impl(BranchIf& br_if, Instance& instance,
                             Continuation next, Value cond);

    BranchIf(const grammar::BranchIf& br_if, std::vector<Operation>& targets);

    Continuation action(Instance& instance) override;

private:
    Operation target_;
};

class BranchTable : public TaggedOperation<BranchTable> {
public:
    BranchTable(const grammar::BranchTable& br_table,
                std::vector<Operation>& targets);

    Continuation action(Instance& instance) override;

private:
    std::vector<uint32_t> label_indices_;
    std::vector<Operation> containers_;
};

class Call : public TaggedOperation<Call> {
public:
    Call(const grammar::Call& call);
    Call(uint32_t func_idx, size_t addr);

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public TaggedOperation<Epilogue> {
    public:
        Epilogue(uint32_t func_idx, size_t addr);

        Continuation action(Instance& instance) override;

    private:
        uint32_t func_idx_;
    };

    uint32_t func_idx_;
};

class CallIndirect : public TaggedOperation<CallIndirect> {
public:
    CallIndirect(const grammar::CallIndirect& call_indirect,
                 const std::vector<FunctionType>& func_types);
    CallIndirect(size_t signature);

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public OperationBase {
    public:
        Epilogue(size_t idx, Function& func, CallIndirect& parent);

        Continuation action(Instance& instance) override;

    private:
        size_t idx_;
        Function& func_;
        CallIndirect& parent_;
    };

    size_t signature_;

    std::vector<Operation> epilogues_;
    std::stack<size_t> free_list_;

    const Operation& createEpilogue(Function& func);
    void destroyEpilogue(size_t idx);
};

class Return : public TaggedOperation<Return> {
public:
    Continuation action(Instance& instance) override;
};

/***************************/
/* Parametric Instructions */
/***************************/

class Drop : public TaggedOperation<Drop> {
public:
    Drop(const grammar::Drop& drop);

    Continuation action(Instance& instance) override;
};

class Select : public TaggedOperation<Select> {
public:
    Continuation action(Instance& instance) override;
};

/*************************/
/* Variable Instructions */
/*************************/

class LocalGet : public GenericOperation<LocalGet> {
public:
    using InType = Values<0>;
    using OutType = Values<1>;

    static OutType impl(LocalGet& local_get, Instance& instance);

    LocalGet(const grammar::LocalGet& local_get)
        : GenericOperation<LocalGet>(local_get.getAddress()),
          local_idx_(local_get.getLocalIdx()) {}

private:
    uint32_t local_idx_;
};

class LocalSet : public GenericOperation<LocalSet> {
public:
    using InType = Values<1>;
    using OutType = Values<0>;

    static OutType impl(LocalSet& local_set, Instance& instance, Value in);

    LocalSet(const grammar::LocalSet& local_set)
        : GenericOperation<LocalSet>(local_set.getAddress()),
          local_idx_(local_set.getLocalIdx()) {}

private:
    uint32_t local_idx_;
};

class LocalTee : public GenericOperation<LocalTee> {
public:
    using InType = std::tuple<Value>;
    using OutType = Value;

    static OutType impl(LocalTee& local_tee, Instance& instance, Value in);

    LocalTee(const grammar::LocalTee& local_tee)
        : GenericOperation<LocalTee>(local_tee.getAddress()),
          local_idx_(local_tee.getLocalIdx()){};

private:
    uint32_t local_idx_;
};

class GlobalGet : public TaggedOperation<GlobalGet> {
public:
    GlobalGet(const grammar::GlobalGet& global_get);

    Continuation action(Instance& instance) override;

private:
    uint32_t global_idx_;
};

class GlobalSet : public OperationBase {
public:
    GlobalSet(const grammar::GlobalSet& global_set);

    Continuation action(Instance& instance) override;

private:
    uint32_t global_idx_;
};

/************************/
/* Numeric Instructions */
/************************/

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

    static OutType impl(I32ReinterpretF32& i32_reinterpret_f32, Instance& instance, Value in);
};

class I64ReinterpretF64 : public TaggedOperation<I64ReinterpretF64> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(I64ReinterpretF64& i64_reinterpret_f64, Instance& instance, Value in);
};

class F32ReinterpretI32 : public GenericOperation<F32ReinterpretI32> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(F32ReinterpretI32& f32_reinterpret_i32, Instance& instance, Value in);
};

class F64ReinterpretI64 : public GenericOperation<F64ReinterpretI64> {
public:
    using InType = Values<1>;
    using OutType = Values<1>;

    static OutType impl(F64ReinterpretI64& f64_reinterpret_i64, Instance& instance, Value in);
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

/***********************/
/* Memory Instructions */
/***********************/

class I32Load : public TaggedOperation<I32Load> {
public:
    I32Load(const grammar::I32Load& i32_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load : public TaggedOperation<I64Load> {
public:
    I64Load(const grammar::I64Load& i64_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class F32Load : public OperationBase {
public:
    F32Load(const grammar::F32Load& f32_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class F64Load : public OperationBase {
public:
    F64Load(const grammar::F64Load& f64_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load8Signed : public TaggedOperation<I32Load8Signed> {
public:
    I32Load8Signed(const grammar::I32Load8Signed& i32_load8_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load8Unsigned : public TaggedOperation<I32Load8Unsigned> {
public:
    I32Load8Unsigned(const grammar::I32Load8Unsigned& i32_load8_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load16Signed : public TaggedOperation<I32Load16Signed> {
public:
    I32Load16Signed(const grammar::I32Load16Signed& i32_load16_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load16Unsigned : public TaggedOperation<I32Load16Unsigned> {
public:
    I32Load16Unsigned(const grammar::I32Load16Unsigned& i32_load16_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load8Signed : public OperationBase {
public:
    I64Load8Signed(const grammar::I64Load8Signed& i64_load8_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load8Unsigned : public OperationBase {
public:
    I64Load8Unsigned(const grammar::I64Load8Unsigned& i64_load8_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load16Signed : public OperationBase {
public:
    I64Load16Signed(const grammar::I64Load16Signed& i64_load16_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load16Unsigned : public OperationBase {
public:
    I64Load16Unsigned(const grammar::I64Load16Unsigned& i64_load16_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load32Signed : public OperationBase {
public:
    I64Load32Signed(const grammar::I64Load32Signed& i64_load32_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load32Unsigned : public OperationBase {
public:
    I64Load32Unsigned(const grammar::I64Load32Unsigned& i64_load32_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Store : public TaggedOperation<I32Store> {
public:
    I32Store(const grammar::I32Store& i32_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Store : public TaggedOperation<I64Store> {
public:
    I64Store(const grammar::I64Store& i64_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class F32Store : public OperationBase {
public:
    F32Store(const grammar::F32Store& f32_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class F64Store : public OperationBase {
public:
    F64Store(const grammar::F64Store& f64_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Store8 : public OperationBase {
public:
    I32Store8(const grammar::I32Store8& i32_store8);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Store16 : public OperationBase {
public:
    I32Store16(const grammar::I32Store16& i32_store16);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Store8 : public OperationBase {
public:
    I64Store8(const grammar::I64Store8& i64_store8);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Store16 : public OperationBase {
public:
    I64Store16(const grammar::I64Store16& i64_store16);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Store32 : public OperationBase {
public:
    I64Store32(const grammar::I64Store32& i64_store32);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class MemoryGrow : public OperationBase {
public:
    MemoryGrow(const grammar::MemoryGrow& mem_grow);

    Continuation action(Instance& instance) override;
};

class MemoryInit : public OperationBase {
public:
    MemoryInit(const grammar::MemoryInit& mem_init);

    Continuation action(Instance& instance) override;

private:
    uint32_t segment_idx_;
};

class DataDrop : public OperationBase {
public:
    DataDrop(const grammar::DataDrop& data_drop);

    Continuation action(Instance& instance) override;

private:
    uint32_t segment_idx_;
};

class MemoryCopy : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class MemoryFill : public OperationBase {
public:
    MemoryFill(const grammar::MemoryFill& mem_fill)
        : OperationBase(mem_fill.getAddress()) {}

    Continuation action(Instance& instance) override;
};

/******************************/
/* Atomic Memory Instructions */
/******************************/

class AtomicNotify : public OperationBase {
public:
    AtomicNotify(const grammar::AtomicNotify& atomic_notify);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicWait32 : public OperationBase {
public:
    AtomicWait32(const grammar::AtomicWait32& atomic_wait32);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;

    Operation idle_ = std::make_shared<Nop>(addr_);
};

class AtomicLoad : public OperationBase {
public:
    AtomicLoad(const grammar::AtomicLoad& atomic_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicLoad8Unsigned : public OperationBase {
public:
    AtomicLoad8Unsigned(const grammar::AtomicLoad8Unsigned& atomic_load8_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicStore : public OperationBase {
public:
    AtomicStore(const grammar::AtomicStore& atomic_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicStore8 : public OperationBase {
public:
    AtomicStore8(const grammar::AtomicStore8& atomic_store8);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicAdd : public OperationBase {
public:
    AtomicAdd(const grammar::AtomicAdd& atomic_add);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicExchange8Unsigned : public OperationBase {
public:
    AtomicExchange8Unsigned(
        const grammar::AtomicExchange8Unsigned& atomic_xchg8_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicCompareExchange : public OperationBase {
public:
    AtomicCompareExchange(const grammar::AtomicCompareExchange& atomic_cmpxchg);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

} // namespace runtime
