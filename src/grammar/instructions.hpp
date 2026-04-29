#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "grammar/types.hpp"
#include "grammar/values.hpp"
#include "util/error_handling.hpp"

namespace grammar {

class InstructionBase;
using Instruction = std::shared_ptr<InstructionBase>;

/**************/
/* Expression */
/**************/

struct Expression {
public:
    static Expected<Expression> parse(std::istream& in, size_t code_start);

    const std::vector<Instruction>& getInstructions() const {
        return instructions_;
    }
    const Instruction& getTerminator() const { return terminator_; }

    std::string toString() const;

private:
    std::vector<Instruction> instructions_;
    Instruction terminator_;

    Expression(std::vector<Instruction>&& instructions,
               Instruction&& terminator)
        : instructions_(std::move(instructions)),
          terminator_(std::move(terminator)) {}
};

/****************/
/* Instructions */
/****************/

class InstructionBase {
public:
    static Expected<Instruction> parse(std::istream& in, size_t code_start);

    virtual ~InstructionBase() = default;

    uint8_t getOpcode() const { return opcode_; }
    size_t getAddress() const { return addr_; }

    virtual std::string toString() const = 0;

    template <class Derived> bool is() const {
        return opcode_ == Derived::OPCODE;
    }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }

protected:
    InstructionBase(uint8_t opcode) : opcode_(opcode) {}
    InstructionBase(uint8_t opcode, size_t addr)
        : opcode_(opcode), addr_(addr) {}

private:
    uint8_t opcode_;
    size_t addr_;
};

/************************/
/* Control Instructions */
/************************/

class Unreachable : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x00;

    Unreachable(size_t addr) : InstructionBase(OPCODE, addr) {}

    std::string toString() const override { return "unreachable"; }
};

class Nop : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x01;

    Nop() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "nop"; }
};

class BlockType {
public:
    static Expected<BlockType> parse(std::istream& in);

    std::string toString() const;

private:
    std::optional<ValueType> val_type_opt_;

    BlockType(std::optional<ValueType>&& val_type_opt)
        : val_type_opt_(std::move(val_type_opt)) {}
};

class Block : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x02;

    static Expected<Block> parse(std::istream& in, size_t code_start);

    const std::vector<Instruction>& getInstruction() const {
        return instructions_;
    }

    std::string toString() const override;

private:
    BlockType block_type_;
    std::vector<Instruction> instructions_;

    Block(const BlockType& block_type, std::vector<Instruction>&& instructions)
        : InstructionBase(OPCODE), block_type_(block_type),
          instructions_(std::move(instructions)) {}
};

class Loop : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x03;

    static Expected<Loop> parse(std::istream& in, size_t addr,
                                size_t code_start);

    const std::vector<Instruction>& getInstruction() const {
        return instructions_;
    }

    std::string toString() const override;

private:
    BlockType block_type_;
    std::vector<Instruction> instructions_;

    Loop(const BlockType& block_type, std::vector<Instruction>&& instructions,
         size_t addr)
        : InstructionBase(OPCODE, addr), block_type_(block_type),
          instructions_(std::move(instructions)) {}
};

class IfElse : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x04;

    static Expected<IfElse> parse(std::istream& in, size_t addr,
                                  size_t code_start);

    const Expression& getThenExpr() const { return then_expr_; }
    const Expression& getElseExpr() const { return *else_expr_; }

    bool hasElseExpression() const { return else_expr_.has_value(); }

    std::string toString() const override;

private:
    Expression then_expr_;
    std::optional<Expression> else_expr_;

    IfElse(Expression&& then_expr, size_t addr)
        : InstructionBase(OPCODE, addr), then_expr_(std::move(then_expr)) {}
    IfElse(Expression&& then_expr, Expression&& else_expr, size_t addr)
        : InstructionBase(OPCODE, addr), then_expr_(std::move(then_expr)),
          else_expr_(std::move(else_expr)) {}
};

class Else : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x05;

    Else() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "else"; }
};

class Return : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x0F;

    Return() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "return"; }
};

class End : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x0B;

    End() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "end"; }
};

class Branch : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x0C;

    static Expected<Branch> parse(std::istream& in);

    uint32_t getLabelIdx() const { return label_idx_; }

    std::string toString() const override;

private:
    uint32_t label_idx_;

    Branch(uint32_t label_idx)
        : InstructionBase(OPCODE), label_idx_(label_idx) {}
};

class BranchIf : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x0D;

    static Expected<BranchIf> parse(std::istream& in, size_t addr);

    uint32_t getLabelIdx() const { return label_idx_; }

    std::string toString() const override;

private:
    uint32_t label_idx_;

    BranchIf(uint32_t label_idx, size_t addr)
        : InstructionBase(OPCODE, addr), label_idx_(label_idx) {}
};

class BranchTable : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x0E;

    static Expected<BranchTable> parse(std::istream& in);

    const std::vector<uint32_t>& getLabelIndices() const {
        return label_indices_;
    }

    std::string toString() const override;

private:
    std::vector<uint32_t> label_indices_;

    BranchTable(std::vector<uint32_t>&& label_indices)
        : InstructionBase(OPCODE), label_indices_(std::move(label_indices)) {}
};

class Call : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x10;

    static Expected<Call> parse(std::istream& in, size_t addr);

    uint32_t getFuncIdx() const { return func_idx_; }

    std::string toString() const override;

private:
    uint32_t func_idx_;

    Call(uint32_t func_idx, size_t addr)
        : InstructionBase(OPCODE, addr), func_idx_(func_idx) {}
};

class CallIndirect : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x11;

    static Expected<CallIndirect> parse(std::istream& in, size_t addr);

    uint32_t getTypeIdx() const { return type_idx_; }
    uint32_t getTableIdx() const { return table_idx_; }

    std::string toString() const override;

private:
    uint32_t type_idx_;
    uint32_t table_idx_;

    CallIndirect(uint32_t type_idx, uint32_t table_idx, size_t addr)
        : InstructionBase(OPCODE, addr), type_idx_(type_idx),
          table_idx_(table_idx) {}
};

/*************************/
/* Variable Instructions */
/*************************/

class LocalGet : public InstructionBase {
public:
    static constexpr uint32_t OPCODE = 0x20;

    static Expected<LocalGet> parse(std::istream& in, size_t addr);

    uint32_t getLocalIdx() const { return local_idx_; }

    std::string toString() const override;

private:
    uint32_t local_idx_;

    LocalGet(uint32_t local_idx, size_t addr)
        : InstructionBase(OPCODE, addr), local_idx_(local_idx) {}
};

class LocalSet : public InstructionBase {
public:
    static constexpr uint32_t OPCODE = 0x21;

    static Expected<LocalSet> parse(std::istream& in, size_t addr);

    uint32_t getLocalIdx() const { return local_idx_; }

    std::string toString() const override;

private:
    uint32_t local_idx_;

    LocalSet(uint32_t local_idx, size_t addr)
        : InstructionBase(OPCODE, addr), local_idx_(local_idx) {}
};

class LocalTee : public InstructionBase {
public:
    static constexpr uint32_t OPCODE = 0x22;

    static Expected<LocalTee> parse(std::istream& in, size_t addr);

    uint32_t getLocalIdx() const { return local_idx_; }

    std::string toString() const override;

private:
    uint32_t local_idx_;

    LocalTee(uint32_t local_idx, size_t addr)
        : InstructionBase(OPCODE, addr), local_idx_(local_idx) {}
};

class GlobalGet : public InstructionBase {
public:
    static constexpr uint32_t OPCODE = 0x23;

    static Expected<GlobalGet> parse(std::istream& in);

    uint32_t getGlobalIdx() const { return global_idx_; }

    std::string toString() const override;

private:
    uint32_t global_idx_;

    GlobalGet(uint32_t global_idx)
        : InstructionBase(OPCODE), global_idx_(global_idx) {}
};

class GlobalSet : public InstructionBase {
public:
    static constexpr uint32_t OPCODE = 0x24;

    static Expected<GlobalSet> parse(std::istream& in);

    uint32_t getGlobalIdx() const { return global_idx_; }

    std::string toString() const override;

private:
    uint32_t global_idx_;

    GlobalSet(uint32_t global_idx)
        : InstructionBase(OPCODE), global_idx_(global_idx) {}
};

/***************************/
/* Parametric Instructions */
/***************************/

class Drop : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x1A;

    Drop(size_t ip) : InstructionBase(OPCODE, ip) {}

    std::string toString() const override { return "drop"; }
};

class Select : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x1B;

    Select() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "select"; }
};

/************************/
/* Numeric Instructions */
/************************/

class I32Const : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x41;

    static Expected<I32Const> parse(std::istream& in, size_t addr);

    int32_t getVal() const { return i_; }

    std::string toString() const override;

private:
    int32_t i_;

    I32Const(int32_t i, size_t addr) : InstructionBase(OPCODE, addr), i_(i) {}
};

class I64Const : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x42;

    static Expected<I64Const> parse(std::istream& in);

    int64_t getVal() const { return i_; }

    std::string toString() const override;

private:
    int64_t i_;

    I64Const(int64_t i) : InstructionBase(OPCODE), i_(i) {}
};

class F32Const : public InstructionBase, public F32 {
public:
    static constexpr uint8_t OPCODE = 0x43;

    static Expected<F32Const> parse(std::istream& in);

    std::string toString() const override;

private:
    F32Const(F32&& f32) : InstructionBase(OPCODE), F32(std::move(f32)) {}
};

class F64Const : public InstructionBase, public F64 {
public:
    static constexpr uint8_t OPCODE = 0x44;

    static Expected<F64Const> parse(std::istream& in);

    std::string toString() const override;

private:
    F64Const(F64&& f64) : InstructionBase(OPCODE), F64(std::move(f64)) {}
};

class I32EqualZero : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x45;

    I32EqualZero() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.eqz"; }
};

class I32Equal : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x46;

    I32Equal() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.eq"; }
};

class I32NotEqual : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x47;

    I32NotEqual() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.ne"; }
};

class I32LessThanSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x48;

    I32LessThanSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.lt_s"; }
};

class I32LessThanUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x49;

    I32LessThanUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.lt_u"; }
};

class I32GreaterThanSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4A;

    I32GreaterThanSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.gt_s"; }
};

class I32GreaterThanUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4B;

    I32GreaterThanUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.gt_u"; }
};

class I32LessEqualSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4C;

    I32LessEqualSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.le_s"; }
};

class I32LessEqualUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4D;

    I32LessEqualUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.le_u"; }
};

class I32GreaterEqualSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4E;

    I32GreaterEqualSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.ge_s"; }
};

class I32GreaterEqualUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4F;

    I32GreaterEqualUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.ge_u"; }
};

class I64EqualZero : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x50;

    I64EqualZero() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.eqz"; }
};

class I64Equal : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x51;

    I64Equal() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.eq"; }
};

class I64NotEqual : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x52;

    I64NotEqual() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.ne"; }
};

class I64LessThanSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x53;

    I64LessThanSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.lt_s"; }
};

class I64LessThanUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x54;

    I64LessThanUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.lt_u"; }
};

class I64GreaterThanSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x55;

    I64GreaterThanSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.gt_s"; }
};

class I64GreaterThanUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x56;

    I64GreaterThanUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.gt_u"; }
};

class I64LessEqualSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x57;

    I64LessEqualSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.le_s"; }
};

class I64LessEqualUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x58;

    I64LessEqualUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.le_u"; }
};

class I64GreaterEqualSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x59;

    I64GreaterEqualSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.ge_s"; }
};

class I64GreaterEqualUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x5A;

    I64GreaterEqualUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.ge_u"; }
};

class F32Equal : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x5B;

    F32Equal() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.eq"; }
};

class F32NotEqual : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x5C;

    F32NotEqual() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.ne"; }
};

class F32LessThan : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x5D;

    F32LessThan() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.lt"; }
};

class F32GreaterThan : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x5E;

    F32GreaterThan() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.gt"; }
};

class F32LessEqual : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x5F;

    F32LessEqual() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.le"; }
};

class F32GreaterEqual : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x60;

    F32GreaterEqual() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.ge"; }
};

class F64Equal : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x61;

    F64Equal() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.eq"; }
};

class F64NotEqual : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x62;

    F64NotEqual() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.ne"; }
};

class F64LessThan : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x63;

    F64LessThan() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.lt"; }
};

class F64GreaterThan : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x64;

    F64GreaterThan() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.gt"; }
};

class F64LessEqual : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x65;

    F64LessEqual() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.le"; }
};

class F64GreaterEqual : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x66;

    F64GreaterEqual() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.ge"; }
};

class I32CountLeadingZeros : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x67;

    I32CountLeadingZeros() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.clz"; }
};

class I32CountTrailingZeros : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x68;

    I32CountTrailingZeros() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.ctz"; }
};

class I32PopCount : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x69;

    I32PopCount() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.popcnt"; }
};

class I32Add : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6A;

    I32Add(size_t addr) : InstructionBase(OPCODE, addr) {}

    std::string toString() const override { return "i32.add"; }
};

class I32Sub : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6B;

    I32Sub(size_t addr) : InstructionBase(OPCODE, addr) {}

    std::string toString() const override { return "i32.sub"; }
};

class I32Mul : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6C;

    I32Mul() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.mul"; }
};

class I32DivideSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6D;

    I32DivideSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.div_s"; }
};

class I32DivideUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6E;

    I32DivideUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.div_u"; }
};

class I32RemainderSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6F;

    I32RemainderSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.rem_s"; }
};

class I32RemainderUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x70;

    I32RemainderUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.rem_u"; }
};

class I32And : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x71;

    I32And() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.and"; }
};

class I32Or : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x72;

    I32Or() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.or"; }
};

class I32Xor : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x73;

    I32Xor() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.xor"; }
};

class I32ShiftLeft : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x74;

    I32ShiftLeft() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.shl"; }
};

class I32ShiftRightSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x75;

    I32ShiftRightSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.shr_s"; }
};

class I32ShiftRightUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x76;

    I32ShiftRightUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.shr_u"; }
};

class I32RotateLeft : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x77;

    I32RotateLeft() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.rotl"; }
};

class I32RotateRight : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x78;

    I32RotateRight() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.rotr"; }
};

class I64CountLeadingZeros : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x79;

    I64CountLeadingZeros() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.clz"; }
};

class I64CountTrailingZeros : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x7A;

    I64CountTrailingZeros() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.ctz"; }
};

class I64Add : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x7C;

    I64Add() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.add"; }
};

class I64Sub : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x7D;

    I64Sub() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.sub"; }
};

class I64Mul : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x7E;

    I64Mul() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.mul"; }
};

class I64DivideSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x7F;

    I64DivideSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.div_s"; }
};

class I64DivideUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x80;

    I64DivideUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.div_u"; }
};

class I64RemainderSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x81;

    I64RemainderSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.rem_u"; }
};

class I64RemainderUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x82;

    I64RemainderUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.rem_u"; }
};

class I64And : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x83;

    I64And() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.and"; }
};

class I64Or : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x84;

    I64Or() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.or"; }
};

class I64Xor : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x85;

    I64Xor() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.xor"; }
};

class I64ShiftLeft : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x86;

    I64ShiftLeft() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.shl"; }
};

class I64ShiftRightSigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x87;

    I64ShiftRightSigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.shr_s"; }
};

class I64ShiftRightUnsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x88;

    I64ShiftRightUnsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.shr_u"; }
};

class I64RotateLeft : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x89;

    I64RotateLeft() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.rotl"; }
};

class I64RotateRight : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x8A;

    I64RotateRight() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.rotr"; }
};

class F32Mul : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x94;

    F32Mul() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.mul"; }
};

class F32Div : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x95;

    F32Div() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.div"; }
};

class F64Mul : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xA2;

    F64Mul() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.mul"; }
};

class F64Div : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xA3;

    F64Div() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.div"; }
};

class F64CopySign : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xA6;

    F64CopySign() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.copysign"; }
};

class I32WrapI64 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xA7;

    I32WrapI64() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.wrap_i64"; }
};

class I64ExtendI32Signed : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xAC;

    I64ExtendI32Signed() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.extend_i32_s"; }
};

class I64ExtendI32Unsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xAD;

    I64ExtendI32Unsigned() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.extend_i32_u"; }
};

class F32ConvertI32Signed : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xB2;

    F32ConvertI32Signed() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.convert_i32_s"; }
};

class F32DemoteF64 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xB6;

    F32DemoteF64() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.demote_f64"; }
};

class F64ConvertI32Signed : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xB7;

    F64ConvertI32Signed() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.convert_i32_s"; }
};

class F64PromoteF32 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xBB;

    F64PromoteF32() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.promote_f32"; }
};

class I32ReinterpretF32 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xBC;

    I32ReinterpretF32() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.reinterpret_f32"; }
};

class I64ReinterpretF64 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xBD;

    I64ReinterpretF64() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.reinterpret_f64"; }
};

class F32ReinterpretI32 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xBE;

    F32ReinterpretI32() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f32.reinterpret_i32"; }
};

class F64ReinterpretI64 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xBF;

    F64ReinterpretI64() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "f64.reinterpret_i64"; }
};

class I32Extend8Signed : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xC0;

    I32Extend8Signed() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.extend8_s"; }
};

class I32Extend16Signed : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xC1;

    I32Extend16Signed() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.extend16_s"; }
};

class I64Extend8Signed : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xC2;

    I64Extend8Signed() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.extend8_s"; }
};

class I64Extend16Signed : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xC3;

    I64Extend16Signed() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.extend16_s"; }
};

class I64Extend32Signed : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xC4;

    I64Extend32Signed() : InstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.extend32_s"; }
};

/***********************/
/* Memory Instructions */
/***********************/

class MemoryArgument {
public:
    static Expected<MemoryArgument> parse(std::istream& in);

    uint32_t getOffset() const { return offset_; }
    uint32_t getAlign() const { return align_; }

    std::string toString() const;

private:
    uint32_t align_;
    uint32_t offset_;

    MemoryArgument(uint32_t align, uint32_t offset)
        : align_(align), offset_(offset) {}
};

class I32Load : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x28;

    static Expected<I32Load> parse(std::istream& in, size_t addr);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Load(const MemoryArgument& mem_arg, size_t addr)
        : InstructionBase(OPCODE, addr), mem_arg_(mem_arg) {}
};

class I64Load : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x29;

    static Expected<I64Load> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I64Load(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class F32Load : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x2A;

    static Expected<F32Load> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    F32Load(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class F64Load : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x2B;

    static Expected<F64Load> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    F64Load(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I32Load8Signed : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x2C;

    static Expected<I32Load8Signed> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Load8Signed(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I32Load8Unsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x2D;

    static Expected<I32Load8Unsigned> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Load8Unsigned(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I32Load16Signed : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x2E;

    static Expected<I32Load16Signed> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Load16Signed(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I32Load16Unsigned : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x2F;

    static Expected<I32Load16Unsigned> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Load16Unsigned(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I64Load8Signed : public InstructionBase, public MemoryArgument {
public:
    static constexpr uint8_t OPCODE = 0x30;

    static Expected<I64Load8Signed> parse(std::istream& in);

    std::string toString() const override;

private:
    I64Load8Signed(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), MemoryArgument(mem_arg) {}
};

class I64Load8Unsigned : public InstructionBase, public MemoryArgument {
public:
    static constexpr uint8_t OPCODE = 0x31;

    static Expected<I64Load8Unsigned> parse(std::istream& in);

    std::string toString() const override;

private:
    I64Load8Unsigned(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), MemoryArgument(mem_arg) {}
};

class I64Load16Signed : public InstructionBase, public MemoryArgument {
public:
    static constexpr uint8_t OPCODE = 0x32;

    static Expected<I64Load16Signed> parse(std::istream& in);

    std::string toString() const override;

private:
    I64Load16Signed(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), MemoryArgument(mem_arg) {}
};

class I64Load16Unsigned : public InstructionBase, public MemoryArgument {
public:
    static constexpr uint8_t OPCODE = 0x33;

    static Expected<I64Load16Unsigned> parse(std::istream& in);

    std::string toString() const override;

private:
    I64Load16Unsigned(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), MemoryArgument(mem_arg) {}
};

class I64Load32Signed : public InstructionBase, public MemoryArgument {
public:
    static constexpr uint8_t OPCODE = 0x34;

    static Expected<I64Load32Signed> parse(std::istream& in);

    std::string toString() const override;

private:
    I64Load32Signed(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), MemoryArgument(mem_arg) {}
};

class I64Load32Unsigned : public InstructionBase, public MemoryArgument {
public:
    static constexpr uint8_t OPCODE = 0x35;

    static Expected<I64Load32Unsigned> parse(std::istream& in);

    std::string toString() const override;

private:
    I64Load32Unsigned(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), MemoryArgument(mem_arg) {}
};

class I32Store : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x36;

    static Expected<I32Store> parse(std::istream& in, size_t addr);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Store(const MemoryArgument& mem_arg, size_t addr)
        : InstructionBase(OPCODE, addr), mem_arg_(mem_arg) {}
};

class I64Store : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x37;

    static Expected<I64Store> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I64Store(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class F32Store : public InstructionBase, public MemoryArgument {
public:
    static constexpr uint8_t OPCODE = 0x38;

    static Expected<F32Store> parse(std::istream& in);

    std::string toString() const override;

private:
    F32Store(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), MemoryArgument(mem_arg) {}
};

class F64Store : public InstructionBase, public MemoryArgument {
public:
    static constexpr uint8_t OPCODE = 0x39;

    static Expected<F64Store> parse(std::istream& in);

    std::string toString() const override;

private:
    F64Store(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), MemoryArgument(mem_arg) {}
};

class I32Store8 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x3A;

    static Expected<I32Store8> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Store8(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I32Store16 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x3B;

    static Expected<I32Store16> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Store16(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I64Store8 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x3C;

    static Expected<I64Store8> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I64Store8(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I64Store16 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x3D;

    static Expected<I64Store16> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I64Store16(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I64Store32 : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x3E;

    static Expected<I64Store32> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I64Store32(const MemoryArgument& mem_arg)
        : InstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class MemoryGrow : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x40;

    static Expected<MemoryGrow> parse(std::istream& in, size_t addr);

    std::string toString() const override;

private:
    MemoryGrow(size_t addr) : InstructionBase(OPCODE, addr) {}
};

class MemoryIntstructionBase : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xFC;

    static Expected<Instruction> parse(std::istream& in, size_t addr);

    uint32_t getMemoryOpcode() const { return mem_opcode_; }

    template <class Derived> bool is() const {
        return mem_opcode_ == Derived::MEM_OPCODE;
    }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }

protected:
    MemoryIntstructionBase(uint32_t mem_id, size_t addr = 0)
        : InstructionBase(OPCODE, addr), mem_opcode_(mem_id) {}

private:
    uint32_t mem_opcode_;
};

class MemoryInit : public MemoryIntstructionBase {
public:
    static constexpr uint32_t MEM_OPCODE = 0x08;

    static Expected<MemoryInit> parse(std::istream& in);

    uint32_t getSegmentIdx() const { return segment_idx_; }

    std::string toString() const override;

private:
    uint32_t segment_idx_;

    MemoryInit(uint32_t segment_idx)
        : MemoryIntstructionBase(MEM_OPCODE), segment_idx_(segment_idx) {}
};

class DataDrop : public MemoryIntstructionBase {
public:
    static constexpr uint32_t MEM_OPCODE = 0x09;

    static Expected<DataDrop> parse(std::istream& in);

    uint32_t getSegmentIdx() const { return segment_idx_; }

    std::string toString() const override;

private:
    uint32_t segment_idx_;

    DataDrop(uint32_t segment_idx)
        : MemoryIntstructionBase(MEM_OPCODE), segment_idx_(segment_idx) {}
};

class MemoryCopy : public MemoryIntstructionBase {
public:
    static constexpr uint32_t MEM_OPCODE = 0x0A;

    static Expected<MemoryCopy> parse(std::istream& in);

    std::string toString() const override { return "memory.copy 0 0"; }

private:
    MemoryCopy() : MemoryIntstructionBase(MEM_OPCODE) {}
};

class MemoryFill : public MemoryIntstructionBase {
public:
    static constexpr uint32_t MEM_OPCODE = 0x0B;

    static Expected<MemoryFill> parse(std::istream& in, size_t addr);

    std::string toString() const override { return "memory.fill"; }

private:
    MemoryFill(size_t addr) : MemoryIntstructionBase(MEM_OPCODE, addr) {}
};

/******************************/
/* Atomic Memory Instructions */
/******************************/

class AtomicIntstructionBase : public InstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xFE;

    static Expected<Instruction> parse(std::istream& in);

    uint32_t getAtomicOpcode() const { return atomic_opcode_; }

    template <class Derived> bool is() const {
        return atomic_opcode_ == Derived::ATOMIC_OPCODE;
    }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }

protected:
    AtomicIntstructionBase(uint32_t atomic_opcode)
        : InstructionBase(OPCODE), atomic_opcode_(atomic_opcode) {}

private:
    uint32_t atomic_opcode_;
};

class AtomicNotify : public AtomicIntstructionBase {
public:
    static constexpr uint32_t ATOMIC_OPCODE = 0x00;

    static Expected<AtomicNotify> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    AtomicNotify(const MemoryArgument& mem_arg)
        : AtomicIntstructionBase(ATOMIC_OPCODE), mem_arg_(mem_arg) {}
};

class AtomicWait32 : public AtomicIntstructionBase {
public:
    static constexpr uint32_t ATOMIC_OPCODE = 0x01;

    static Expected<AtomicWait32> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    AtomicWait32(const MemoryArgument& mem_arg)
        : AtomicIntstructionBase(ATOMIC_OPCODE), mem_arg_(mem_arg) {}
};

class AtomicLoad : public AtomicIntstructionBase {
public:
    static constexpr uint32_t ATOMIC_OPCODE = 0x10;

    static Expected<AtomicLoad> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    AtomicLoad(const MemoryArgument& mem_arg)
        : AtomicIntstructionBase(ATOMIC_OPCODE), mem_arg_(mem_arg) {}
};

class AtomicLoad8Unsigned : public AtomicIntstructionBase {
public:
    static constexpr uint32_t ATOMIC_OPCODE = 0x12;

    static Expected<AtomicLoad8Unsigned> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    AtomicLoad8Unsigned(const MemoryArgument& mem_arg)
        : AtomicIntstructionBase(ATOMIC_OPCODE), mem_arg_(mem_arg) {}
};

class AtomicStore : public AtomicIntstructionBase {
public:
    static constexpr uint32_t ATOMIC_OPCODE = 0x17;

    static Expected<AtomicStore> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    AtomicStore(const MemoryArgument& mem_arg)
        : AtomicIntstructionBase(ATOMIC_OPCODE), mem_arg_(mem_arg) {}
};

class AtomicStore8 : public AtomicIntstructionBase {
public:
    static constexpr uint32_t ATOMIC_OPCODE = 0x19;

    static Expected<AtomicStore8> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    AtomicStore8(const MemoryArgument& mem_arg)
        : AtomicIntstructionBase(ATOMIC_OPCODE), mem_arg_(mem_arg) {}
};

class AtomicAdd : public AtomicIntstructionBase {
public:
    static constexpr uint32_t ATOMIC_OPCODE = 0x1E;

    static Expected<AtomicAdd> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    AtomicAdd(const MemoryArgument& mem_arg)
        : AtomicIntstructionBase(ATOMIC_OPCODE), mem_arg_(mem_arg) {}
};

class AtomicExchange8Unsigned : public AtomicIntstructionBase {
public:
    static constexpr uint32_t ATOMIC_OPCODE = 0x43;

    static Expected<AtomicExchange8Unsigned> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    AtomicExchange8Unsigned(const MemoryArgument& mem_arg)
        : AtomicIntstructionBase(ATOMIC_OPCODE), mem_arg_(mem_arg) {}
};

class AtomicCompareExchange : public AtomicIntstructionBase {
public:
    static constexpr uint32_t ATOMIC_OPCODE = 0x48;

    static Expected<AtomicCompareExchange> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    AtomicCompareExchange(const MemoryArgument& mem_arg)
        : AtomicIntstructionBase(ATOMIC_OPCODE), mem_arg_(mem_arg) {}
};

class AtomicCompareExchange8Unsigned : public AtomicIntstructionBase {
public:
    static constexpr uint32_t ATOMIC_OPCODE = 0x4A;

    static Expected<AtomicCompareExchange8Unsigned> parse(std::istream& in);

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    AtomicCompareExchange8Unsigned(const MemoryArgument& mem_arg)
        : AtomicIntstructionBase(ATOMIC_OPCODE), mem_arg_(mem_arg) {}
};

} // namespace grammar
