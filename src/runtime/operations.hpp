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

    virtual Continuation action(Instance& instance) { return next_.get(); }
    virtual Expected<Continuation> eval(Context& context) const {
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

    Operation next_;
    size_t addr_ = UINT32_MAX;

    OperationBase() = default;
    OperationBase(size_t addr) : addr_(addr) {}

    Continuation trap(Instance& instance, std::string msg, size_t addr) const;
};

template <typename Derived> class TaggedOperation : public OperationBase {
public:
    using OperationBase::OperationBase;

    static TypeId static_type() {
        static int id;
        return static_cast<const void *>(&id);
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
    IfThen(const grammar::IfElse& if_else,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& branch_targets);

    Continuation action(Instance& instance) override;

protected:
    Operation then_;
};

class IfElse : public TaggedOperation<IfElse> {
public:
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

class LocalGet : public TaggedOperation<LocalGet> {
public:
    LocalGet(const grammar::LocalGet& local_get);

    Continuation action(Instance& instance) override;

private:
    uint32_t local_idx_;

    static Value impl(LocalGet& local_get, Instance& instance);

    template <size_t> friend class GenericProducer_LocalGet;

    friend class LocalGet_LocalGet;
    friend class LocalGet_I32Const;
    friend class LocalSet_LocalGet;
    friend class LocalSet_LocalGet_LocalGet;
    friend class I32Const_LocalSet_LocalGet;
};

class LocalSet : public TaggedOperation<LocalSet> {
public:
    LocalSet(const grammar::LocalSet& local_set);

    Continuation action(Instance& instance) override;

private:
    uint32_t local_idx_;

    static void impl(LocalSet& local_set, Instance& instance, Value value);

    friend class LocalSet_LocalGet;
    friend class LocalSet_I32Const;
    friend class LocalSet_I32Const_LocalSet;
    friend class I32Const_LocalSet;
};

class LocalTee : public TaggedOperation<LocalTee> {
public:
    LocalTee(const grammar::LocalTee& local_tee);

    Continuation action(Instance& instance) override;

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

class I32Const : public TaggedOperation<I32Const> {
public:
    I32Const(const grammar::I32Const& i32_const);

    Continuation action(Instance& instance) override;
    Expected<Continuation> eval(Context& context) const override;

private:
    int32_t i_;

    static Value impl(I32Const& i32_const, Instance& instance);

    friend class LocalGet_I32Const;
    friend class LocalSet_I32Const;
    friend class I32Const_LocalSet;
    friend class I32Const_I32Const;
    friend class I32Const_I32Const_I32Const;
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

class I32Add : public TaggedOperation<I32Add> {
public:
    I32Add(const grammar::I32Add& i32_add)
        : TaggedOperation<I32Add>(i32_add.getAddress()) {}

    Continuation action(Instance& instance) override;

private:
    static Value impl(I32Add& i32_add, Instance& instance, const std::array<Value, 2>& in);
};

class I32Sub : public TaggedOperation<I32Sub> {
public:
    I32Sub(const grammar::I32Sub& i32_sub)
        : TaggedOperation<I32Sub>(i32_sub.getAddress()) {}

    Continuation action(Instance& instance) override;
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

class F32Mul : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F32Div : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64Mul : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64Div : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64CopySign : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32WrapI64 : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64ExtendI32Signed : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64ExtendI32Unsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F32ConvertI32Signed : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F32DemoteF64 : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64ConvertI32Signed : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64PromoteF32 : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32ReinterpretF32 : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64ReinterpretF64 : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F32ReinterpretI32 : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class F64ReinterpretI64 : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32Extend8Signed : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32Extend16Signed : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64Extend8Signed : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64Extend16Signed : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64Extend32Signed : public OperationBase {
public:
    Continuation action(Instance& instance) override;
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

class I32Load8Signed : public OperationBase {
public:
    I32Load8Signed(const grammar::I32Load8Signed& i32_load8_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load8Unsigned : public OperationBase {
public:
    I32Load8Unsigned(const grammar::I32Load8Unsigned& i32_load8_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load16Signed : public OperationBase {
public:
    I32Load16Signed(const grammar::I32Load16Signed& i32_load16_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load16Unsigned : public OperationBase {
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
