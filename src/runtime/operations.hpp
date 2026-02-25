#pragma once

#include <cassert>
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
    virtual ~OperationBase() = default;

    static Operation
    create(const std::vector<grammar::Instruction>& instructions,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& targets);

    virtual Operation addNext(Operation op);
    void clearNext() { successor_.reset(); }

    virtual Continuation action(Instance& instance) { return successor_.get(); }
    virtual Expected<Continuation> eval(Context& context) const {
        return Unexpected(ERROR("evaluate called on non constant expression"));
    }
    virtual Value produce(Instance& instance) { assert(false); }
    virtual void consume(Instance& instance, Value value) { assert(false); }

    virtual bool isProducer() const { return false; }
    virtual bool isConsumer() const { return false; }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }
    template <class Derived> Derived& as() {
        return reinterpret_cast<Derived&>(*this);
    }

    size_t getAddress() { return addr_; }
    std::string getFormattedAddress(Instance& instance) const;

protected:
    Operation successor_;
    Continuation next_ = nullptr;
    size_t addr_ = UINT32_MAX;

    OperationBase() = default;
    OperationBase(size_t addr) : addr_(addr) {}

    Continuation trap(Instance& instance, std::string msg, size_t addr) const;
};

/**********************/
/* Control Operations */
/**********************/

class Unreachable : public OperationBase {
public:
    Unreachable(const grammar::Unreachable& unreachable);

    Continuation action(Instance& instance) override;
};

class Nop : public OperationBase {
public:
    Nop(const grammar::Nop& nop) : OperationBase(nop.getAddress()) {}
    Nop(size_t addr) : OperationBase(addr) {}

    void setAddress(size_t addr) { addr_ = addr; }
};

class Block : public OperationBase {
public:
    Block(const grammar::Block& block,
          const std::vector<FunctionType>& func_types,
          std::vector<Operation>& branch_targets);

    Operation addNext(Operation next) override {
        if (!next_added_) {
            end_->as<Nop>().setAddress(next->getAddress());
            next_added_ = true;
        }

        end_->addNext(next);
        return shared_from_this();
    }

    Continuation action(Instance& instance) override;

private:
    Operation start_;
    Operation end_ = std::make_shared<Nop>(addr_);
    bool next_added_ = false;
};

class Loop : public OperationBase {
public:
    Loop(const grammar::Loop& loop, const std::vector<FunctionType>& func_types,
         std::vector<Operation>& branch_targets);

    Operation addNext(Operation next) override {
        end_->addNext(next);
        return shared_from_this();
    }

    Continuation action(Instance& instance) override;

private:
    Operation start_ = std::make_shared<Nop>(addr_);
    Operation end_ = std::make_shared<Nop>(addr_);
};

class IfThen : public OperationBase {
public:
    IfThen(const grammar::IfElse& if_else,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& branch_targets);

    Continuation action(Instance& instance) override;

protected:
    Operation then_;
    Operation end_ = std::make_shared<Nop>(addr_);
};

class IfElse : public OperationBase {
public:
    IfElse(const grammar::IfElse& if_else,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& branch_targets);

    Continuation action(Instance& instance) override;

private:
    Operation then_;
    Operation else_;
    Operation end_ = std::make_shared<Nop>(addr_);
};

class Branch : public OperationBase {
public:
    Branch(const grammar::Branch& branch, std::vector<Operation>& targets);

    Operation addNext(Operation) override { return shared_from_this(); }

    Continuation action(Instance& instance) override;

private:
    Operation target_;
};

class BranchIf : public OperationBase {
public:
    BranchIf(const grammar::BranchIf& br_if, std::vector<Operation>& targets);

    Continuation action(Instance& instance) override;

private:
    Operation target_;
};

class BranchTable : public OperationBase {
public:
    BranchTable(const grammar::BranchTable& br_table,
                std::vector<Operation>& targets);

    Operation addNext(Operation) override { return shared_from_this(); }

    Continuation action(Instance& instance) override;

private:
    std::vector<uint32_t> label_indices_;
    std::vector<Operation> containers_;
};

class Call : public OperationBase {
public:
    Call(const grammar::Call& call);
    Call(uint32_t func_idx, size_t addr);

    Operation addNext(Operation next) override {
        epilogue_->addNext(next);
        return shared_from_this();
    }

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public OperationBase {
    public:
        Epilogue(uint32_t func_idx, size_t addr);

        Continuation action(Instance& instance) override;

    private:
        uint32_t func_idx_;
    };

    uint32_t func_idx_;
    Operation epilogue_ = nullptr;
};

class CallIndirect : public OperationBase {
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

class Return : public OperationBase {
public:
    Operation addNext(Operation next) override { return shared_from_this(); }

    Continuation action(Instance& instance) override;
};

/***************************/
/* Parametric Instructions */
/***************************/

class Drop : public OperationBase {
public:
    Drop(const grammar::Drop& drop);

    Continuation action(Instance& instance) override;
};

class Select : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

/*************************/
/* Variable Instructions */
/*************************/

class LocalGet : public OperationBase {
public:
    LocalGet(const grammar::LocalGet& local_get);

    Continuation action(Instance& instance) override;

private:
    uint32_t local_idx_;
};

class LocalSet : public OperationBase {
public:
    LocalSet(const grammar::LocalSet& local_set);

    Continuation action(Instance& instance) override;

private:
    uint32_t local_idx_;
};

class LocalTee : public OperationBase {
public:
    LocalTee(const grammar::LocalTee& local_tee);

    Continuation action(Instance& instance) override;

private:
    uint32_t local_idx_;
};

class GlobalGet : public OperationBase {
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

class I32Const : public OperationBase {
public:
    I32Const(const grammar::I32Const& i32_const);

    Continuation action(Instance& instance) override;
    Expected<Continuation> eval(Context& context) const override;
    Value produce(Instance& instance) override;

    bool isProducer() const override { return true; }

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

class I32EqualZero : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32Equal : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32NotEqual : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32LessThanSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32LessThanUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32GreaterThanSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32GreaterThanUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32LessEqualSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32LessEqualUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32GreaterEqualSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32GreaterEqualUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64EqualZero : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64Equal : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64NotEqual : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64LessThanSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64LessThanUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64GreaterThanSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64GreaterThanUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64LessEqualSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64LessEqualUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64GreaterEqualSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64GreaterEqualUnsigned : public OperationBase {
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

class I32Add : public OperationBase {
public:
    I32Add(const grammar::I32Add& i32_add)
        : OperationBase(i32_add.getAddress()) {}

    Continuation action(Instance& instance) override;
    void consume(Instance& instance, Value right) override;

    bool isConsumer() const override { return true; }
};

class I32Sub : public OperationBase {
public:
    I32Sub(const grammar::I32Sub& i32_sub)
        : OperationBase(i32_sub.getAddress()) {}

    Continuation action(Instance& instance) override;
    void consume(Instance& instance, Value right) override;

    bool isConsumer() const override { return true; }
};

class I32Mul : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32DivideSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32DivideUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32RemainderSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32RemainderUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32And : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32Or : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32Xor : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32ShiftLeft : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32ShiftRightSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I32ShiftRightUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64CountLeadingZeros : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64CountTrailingZeros : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64Add : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64Sub : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64Mul : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64DivideSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64DivideUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64RemainderSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64RemainderUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64And : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64Or : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64Xor : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64ShiftLeft : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64ShiftRightSigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64ShiftRightUnsigned : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64RotateLeft : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class I64RotateRight : public OperationBase {
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

class I32Load : public OperationBase {
public:
    I32Load(const grammar::I32Load& i32_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load : public OperationBase {
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

class I32Store : public OperationBase {
public:
    I32Store(const grammar::I32Store& i32_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Store : public OperationBase {
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
    MemoryFill(const grammar::MemoryFill& mem_fill) : OperationBase(mem_fill.getAddress()) {}

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

/* Optimized Operations */

class Composite : public OperationBase {
public:
    Composite(const Operation& producer, const Operation& consumer)
        : producer_(producer), consumer_(consumer) {}

    Continuation action(Instance& instance) override;

private:
    Operation producer_;
    Operation consumer_;
};

} // namespace runtime
