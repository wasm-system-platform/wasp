#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "grammar/types.hpp"
#include "util/error_handling.hpp"

namespace grammar {

class IntstructionBase;
using Instruction = std::shared_ptr<IntstructionBase>;

class IntstructionBase {
public:
    static Expected<Instruction> parse(std::istream& in);

    virtual ~IntstructionBase() = default;

    uint8_t getOpcode() const { return opcode_; }
    size_t getInstructionPtr() const { return ip_; }

    virtual std::string toString() const = 0;

    template <class Derived> bool is() const {
        return opcode_ == Derived::OPCODE;
    }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }

protected:
    IntstructionBase(uint8_t opcode) : opcode_(opcode) {}
    IntstructionBase(uint8_t opcode, size_t ip) : opcode_(opcode), ip_(ip) {}

private:
    uint8_t opcode_;
    size_t ip_;
};

/************************/
/* Control Instructions */
/************************/

class Unreachable : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x00;

    Unreachable() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "unreachable"; }
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

class Block : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x02;

    static Expected<Block> parse(std::istream& in);

    const std::vector<Instruction>& getInstruction() const {
        return instructions_;
    }

    std::string toString() const override;

private:
    BlockType block_type_;
    std::vector<Instruction> instructions_;

    Block(const BlockType& block_type, std::vector<Instruction>&& instructions)
        : IntstructionBase(OPCODE), block_type_(block_type),
          instructions_(std::move(instructions)) {}
};

class Loop : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x03;

    static Expected<Loop> parse(std::istream& in);

    const std::vector<Instruction>& getInstruction() const {
        return instructions_;
    }

    std::string toString() const override;

private:
    BlockType block_type_;
    std::vector<Instruction> instructions_;

    Loop(const BlockType& block_type, std::vector<Instruction>&& instructions)
        : IntstructionBase(OPCODE), block_type_(block_type),
          instructions_(std::move(instructions)) {}
};

class Return : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x0F;

    Return() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "return"; }
};

class End : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x0B;

    End() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "end"; }
};

class Branch : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x0C;

    static Expected<Branch> parse(std::istream& in);

    uint32_t getLabelIdx() const { return label_idx_; }

    std::string toString() const override;

private:
    uint32_t label_idx_;

    Branch(uint32_t label_idx)
        : IntstructionBase(OPCODE), label_idx_(label_idx) {}
};

class BranchIf : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x0D;

    static Expected<BranchIf> parse(std::istream& in);

    uint32_t getLabelIdx() const { return label_idx_; }

    std::string toString() const override;

private:
    uint32_t label_idx_;

    BranchIf(uint32_t label_idx)
        : IntstructionBase(OPCODE), label_idx_(label_idx) {}
};

class BranchTable : public IntstructionBase {
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
        : IntstructionBase(OPCODE), label_indices_(std::move(label_indices)) {}
};

class Call : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x10;

    static Expected<Call> parse(std::istream& in);

    uint32_t getFuncIdx() const { return func_idx_; }

    std::string toString() const override;

private:
    uint32_t func_idx_;

    Call(uint32_t func_idx) : IntstructionBase(OPCODE), func_idx_(func_idx) {}
};

class CallIndirect : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x11;

    static Expected<CallIndirect> parse(std::istream& in);

    uint32_t getTypeIdx() const { return type_idx_; }
    uint32_t getTableIdx() const { return table_idx_; }

    std::string toString() const override;

private:
    uint32_t type_idx_;
    uint32_t table_idx_;

    CallIndirect(uint32_t type_idx, uint32_t table_idx)
        : IntstructionBase(OPCODE), type_idx_(type_idx), table_idx_(table_idx) {
    }
};

/*************************/
/* Variable Instructions */
/*************************/

class LocalGet : public IntstructionBase {
public:
    static constexpr uint32_t OPCODE = 0x20;

    static Expected<LocalGet> parse(std::istream& in);

    uint32_t getLocalIdx() const { return local_idx_; }

    std::string toString() const override;

private:
    uint32_t local_idx_;

    LocalGet(uint32_t local_idx)
        : IntstructionBase(OPCODE), local_idx_(local_idx) {}
};

class LocalSet : public IntstructionBase {
public:
    static constexpr uint32_t OPCODE = 0x21;

    static Expected<LocalSet> parse(std::istream& in);

    uint32_t getLocalIdx() const { return local_idx_; }

    std::string toString() const override;

private:
    uint32_t local_idx_;

    LocalSet(uint32_t local_idx)
        : IntstructionBase(OPCODE), local_idx_(local_idx) {}
};

class GlobalGet : public IntstructionBase {
public:
    static constexpr uint32_t OPCODE = 0x23;

    static Expected<GlobalGet> parse(std::istream& in);

    uint32_t getGlobalIdx() const { return global_idx_; }

    std::string toString() const override;

private:
    uint32_t global_idx_;

    GlobalGet(uint32_t global_idx)
        : IntstructionBase(OPCODE), global_idx_(global_idx) {}
};

class GlobalSet : public IntstructionBase {
public:
    static constexpr uint32_t OPCODE = 0x24;

    static Expected<GlobalSet> parse(std::istream& in);

    uint32_t getGlobalIdx() const { return global_idx_; }

    std::string toString() const override;

private:
    uint32_t global_idx_;

    GlobalSet(uint32_t global_idx)
        : IntstructionBase(OPCODE), global_idx_(global_idx) {}
};

/***************************/
/* Parametric Instructions */
/***************************/

class Drop : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x1A;

    Drop(size_t ip) : IntstructionBase(OPCODE, ip) {}

    std::string toString() const override { return "drop"; }
};

class Select : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x1B;

    Select() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "select"; }
};

/************************/
/* Numeric Instructions */
/************************/

class I32Const : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x41;

    static Expected<I32Const> parse(std::istream& in);

    int32_t getVal() const { return i_; }

    std::string toString() const override;

private:
    int32_t i_;

    I32Const(int32_t i) : IntstructionBase(OPCODE), i_(i) {}
};

class I64Const : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x42;

    static Expected<I64Const> parse(std::istream& in);

    int64_t getVal() const { return i_; }

    std::string toString() const override;

private:
    int64_t i_;

    I64Const(int64_t i) : IntstructionBase(OPCODE), i_(i) {}
};

class I32EqualZero : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x45;

    I32EqualZero() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.eqz"; }
};

class I32Equal : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x46;

    I32Equal() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.eq"; }
};

class I32NotEqual : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x47;

    I32NotEqual() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.ne"; }
};

class I32LessThanSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x48;

    I32LessThanSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.lt_s"; }
};

class I32LessThanUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x49;

    I32LessThanUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.lt_u"; }
};

class I32GreaterThanSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4A;

    I32GreaterThanSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.gt_s"; }
};

class I32GreaterThanUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4B;

    I32GreaterThanUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.gt_u"; }
};

class I32LessEqualSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4C;

    I32LessEqualSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.le_s"; }
};

class I32LessEqualUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4D;

    I32LessEqualUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.le_u"; }
};

class I32GreaterEqualSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4E;

    I32GreaterEqualSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.ge_s"; }
};

class I32GreaterEqualUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x4F;

    I32GreaterEqualUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.ge_u"; }
};

class I64EqualZero : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x50;

    I64EqualZero() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.eqz"; }
};

class I64Equal : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x51;

    I64Equal() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.eq"; }
};

class I64NotEqual : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x52;

    I64NotEqual() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.ne"; }
};

class I64LessThanSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x53;

    I64LessThanSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.lt_s"; }
};

class I64GreaterThanSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x55;

    I64GreaterThanSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.gt_s"; }
};

class I64GreaterThanUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x56;

    I64GreaterThanUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.gt_u"; }
};

class I64GreaterEqualSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x59;

    I64GreaterEqualSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.ge_s"; }
};

class I32Add : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6A;

    I32Add() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.add"; }
};

class I32Sub : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6B;

    I32Sub() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.sub"; }
};

class I32Mul : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6C;

    I32Mul() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.mul"; }
};

class I32DivideSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6D;

    I32DivideSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.div_s"; }
};

class I32DivideUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6E;

    I32DivideUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.div_u"; }
};

class I32RemainderSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x6F;

    I32RemainderSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.rem_s"; }
};

class I32RemainderUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x70;

    I32RemainderUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.rem_u"; }
};

class I32And : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x71;

    I32And() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.and"; }
};

class I32Or : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x72;

    I32Or() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.or"; }
};

class I32Xor : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x73;

    I32Xor() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.xor"; }
};

class I32ShiftLeft : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x74;

    I32ShiftLeft() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.shl"; }
};

class I32ShiftRightSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x75;

    I32ShiftRightSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.shr_s"; }
};

class I32ShiftRightUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x76;

    I32ShiftRightUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.shr_u"; }
};

class I64Add : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x7C;

    I64Add() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.add"; }
};

class I64Sub : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x7D;

    I64Sub() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.sub"; }
};

class I64Mul : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x7E;

    I64Mul() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.mul"; }
};

class I64DivideSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x7F;

    I64DivideSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.div_s"; }
};

class I64DivideUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x80;

    I64DivideUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.div_u"; }
};

class I64RemainderSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x81;

    I64RemainderSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.rem_u"; }
};

class I64RemainderUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x82;

    I64RemainderUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.rem_u"; }
};

class I64And : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x83;

    I64And() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.and"; }
};

class I64Or : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x84;

    I64Or() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.or"; }
};

class I64Xor : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x85;

    I64Xor() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.xor"; }
};

class I64ShiftLeft : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x86;

    I64ShiftLeft() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.shl"; }
};

class I64ShiftRightSigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x87;

    I64ShiftRightSigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.shr_s"; }
};

class I64ShiftRightUnsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x88;

    I64ShiftRightUnsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.shr_u"; }
};

class I32WrapI64 : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xA7;

    I32WrapI64() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i32.wrap_i64"; }
};

class I64ExtendI32Signed : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xAC;

    I64ExtendI32Signed() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.extend_i32_s"; }
};

class I64ExtendI32Unsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xAD;

    I64ExtendI32Unsigned() : IntstructionBase(OPCODE) {}

    std::string toString() const override { return "i64.extend_i32_u"; }
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

class I32Load : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x28;

    static Expected<I32Load> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Load(const MemoryArgument& mem_arg)
        : IntstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I64Load : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x29;

    static Expected<I64Load> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I64Load(const MemoryArgument& mem_arg)
        : IntstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I32Load8Signed : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x2C;

    static Expected<I32Load8Signed> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Load8Signed(const MemoryArgument& mem_arg)
        : IntstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I32Load8Unsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x2D;

    static Expected<I32Load8Unsigned> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Load8Unsigned(const MemoryArgument& mem_arg)
        : IntstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I32Load16Unsigned : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x2F;

    static Expected<I32Load16Unsigned> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Load16Unsigned(const MemoryArgument& mem_arg)
        : IntstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I32Store : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x36;

    static Expected<I32Store> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Store(const MemoryArgument& mem_arg)
        : IntstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I64Store : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x37;

    static Expected<I64Store> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I64Store(const MemoryArgument& mem_arg)
        : IntstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class F64Store : public IntstructionBase, public MemoryArgument {
public:
    static constexpr uint8_t OPCODE = 0x39;

    static Expected<F64Store> parse(std::istream& in);

    std::string toString() const override;

private:
    F64Store(const MemoryArgument& mem_arg)
        : IntstructionBase(OPCODE), MemoryArgument(mem_arg) {}
};

class I32Store8 : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x3A;

    static Expected<I32Store8> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Store8(const MemoryArgument& mem_arg)
        : IntstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class I32Store16 : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0x3B;

    static Expected<I32Store16> parse(std::istream& in);

    const MemoryArgument& getMemArg() const { return mem_arg_; }

    std::string toString() const override;

private:
    MemoryArgument mem_arg_;

    I32Store16(const MemoryArgument& mem_arg)
        : IntstructionBase(OPCODE), mem_arg_(mem_arg) {}
};

class MemoryIntstructionBase : public IntstructionBase {
public:
    static constexpr uint8_t OPCODE = 0xFC;

    static Expected<Instruction> parse(std::istream& in);

    uint32_t getMemoryOpcode() const { return mem_opcode_; }

    template <class Derived> bool is() const {
        return mem_opcode_ == Derived::MEM_OPCODE;
    }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }

protected:
    MemoryIntstructionBase(uint32_t mem_id)
        : IntstructionBase(OPCODE), mem_opcode_(mem_id) {}

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

    static Expected<MemoryFill> parse(std::istream& in);

    std::string toString() const override { return "memory.fill"; }

private:
    MemoryFill() : MemoryIntstructionBase(MEM_OPCODE) {}
};

/******************************/
/* Atomic Memory Instructions */
/******************************/

class AtomicIntstructionBase : public IntstructionBase {
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
        : IntstructionBase(OPCODE), atomic_opcode_(atomic_opcode) {}

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

/***************/
/* Expressions */
/***************/

class Expression {
public:
    static Expected<Expression> parse(std::istream& in);

    const std::vector<Instruction>& getInstructions() const {
        return instructions_;
    }

    std::string toString() const;

private:
    std::vector<Instruction> instructions_;

    Expression(std::vector<Instruction>&& instructions);
};

} // namespace grammar
