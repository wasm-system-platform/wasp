#include <cassert>
#include <fmt/format.h>
#include <fmt/ranges.h>

#include "grammar/instructions.hpp"
#include "grammar/values.hpp"

namespace grammar {

/**************/
/* Expression */
/**************/

Expected<Expression> Expression::parse(ByteCursor& in, size_t code_start) {
    std::vector<Instruction> instructions;
    Instruction terminator;

    while (true) {
        Expected<Instruction> inst_exp = InstructionBase::parse(in, code_start);
        if (!inst_exp)
            return Unexpected(PROPAGATE(inst_exp));
        const Instruction& inst = *inst_exp;

        if (inst->is<End>() || inst->is<Else>()) {
            terminator = inst;
            break;
        }

        instructions.push_back(inst);
    }

    Expression expr(std::move(instructions), std::move(terminator));
    return expr;
}

std::string Expression::toString() const {
    std::vector<std::string> fmt_instructions;
    fmt_instructions.reserve(instructions_.size());

    for (size_t i = 0; i < instructions_.size(); i++)
        fmt_instructions.push_back(instructions_[i]->toString());

    return fmt::format("{}", fmt::join(fmt_instructions, "\n"));
}

/****************/
/* Instructions */
/****************/

Expected<Instruction> InstructionBase::parse(ByteCursor& in,
                                             size_t code_start) {
    size_t addr = (code_start == UINT32_MAX)
                      ? code_start
                      : in.offset() - code_start;

    uint8_t opcode = in.byte();

    switch (opcode) {
    case Unreachable::OPCODE: {
        return std::make_shared<Unreachable>(addr);
    }
    case Nop::OPCODE: {
        return std::make_shared<Nop>();
    }
    case Block::OPCODE: {
        Expected<Block> block_exp = Block::parse(in, code_start);
        if (!block_exp)
            return Unexpected(PROPAGATE(block_exp));
        return std::make_shared<Block>(*block_exp);
    }
    case Loop::OPCODE: {
        Expected<Loop> loop_exp = Loop::parse(in, addr, code_start);
        if (!loop_exp)
            return Unexpected(PROPAGATE(loop_exp));
        return std::make_shared<Loop>(*loop_exp);
    }
    case IfElse::OPCODE: {
        Expected<IfElse> if_else_exp = IfElse::parse(in, addr, code_start);
        if (!if_else_exp)
            return Unexpected(PROPAGATE(if_else_exp));
        return std::make_shared<IfElse>(*if_else_exp);
    }
    case Else::OPCODE:
        return std::make_shared<Else>();
    case Return::OPCODE:
        return std::make_shared<Return>();
    case End::OPCODE:
        return std::make_shared<End>();
    case Branch::OPCODE: {
        Expected<Branch> br_exp = Branch::parse(in);
        if (!br_exp)
            return Unexpected(PROPAGATE(br_exp));
        return std::make_shared<Branch>(*br_exp);
    }
    case BranchIf::OPCODE: {
        Expected<BranchIf> br_if_exp = BranchIf::parse(in, addr);
        if (!br_if_exp)
            return Unexpected(PROPAGATE(br_if_exp));
        return std::make_shared<BranchIf>(*br_if_exp);
    }
    case BranchTable::OPCODE: {
        Expected<BranchTable> br_table_exp = BranchTable::parse(in);
        if (!br_table_exp)
            return Unexpected(PROPAGATE(br_table_exp));
        return std::make_shared<BranchTable>(*br_table_exp);
    }
    case Call::OPCODE: {
        Expected<Call> call_exp = Call::parse(in, addr);
        if (!call_exp)
            return Unexpected(PROPAGATE(call_exp));
        return std::make_shared<Call>(*call_exp);
    }
    case CallIndirect::OPCODE: {
        Expected<CallIndirect> call_indirect_exp =
            CallIndirect::parse(in, addr);
        if (!call_indirect_exp)
            return Unexpected(PROPAGATE(call_indirect_exp));
        return std::make_shared<CallIndirect>(*call_indirect_exp);
    }
    case LocalGet::OPCODE: {
        Expected<LocalGet> local_get_exp = LocalGet::parse(in, addr);
        if (!local_get_exp)
            return Unexpected(PROPAGATE(local_get_exp));
        return std::make_shared<LocalGet>(*local_get_exp);
    }
    case LocalSet::OPCODE: {
        Expected<LocalSet> local_set_exp = LocalSet::parse(in, addr);
        if (!local_set_exp)
            return Unexpected(PROPAGATE(local_set_exp));
        return std::make_shared<LocalSet>(*local_set_exp);
    }
    case LocalTee::OPCODE: {
        Expected<LocalTee> local_tee_exp = LocalTee::parse(in, addr);
        if (!local_tee_exp)
            return Unexpected(PROPAGATE(local_tee_exp));
        return std::make_shared<LocalTee>(*local_tee_exp);
    }
    case GlobalGet::OPCODE: {
        Expected<GlobalGet> global_get_exp = GlobalGet::parse(in);
        if (!global_get_exp)
            return Unexpected(PROPAGATE(global_get_exp));
        return std::make_shared<GlobalGet>(*global_get_exp);
    }
    case GlobalSet::OPCODE: {
        Expected<GlobalSet> global_set_exp = GlobalSet::parse(in);
        if (!global_set_exp)
            return Unexpected(PROPAGATE(global_set_exp));
        return std::make_shared<GlobalSet>(*global_set_exp);
    }
    case Drop::OPCODE:
        return std::make_shared<Drop>(addr);
    case Select::OPCODE:
        return std::make_shared<Select>();
    case I32Const::OPCODE: {
        Expected<I32Const> i32_const_exp = I32Const::parse(in, addr);
        if (!i32_const_exp)
            return Unexpected(PROPAGATE(i32_const_exp));
        return std::make_shared<I32Const>(*i32_const_exp);
    }
    case I64Const::OPCODE: {
        Expected<I64Const> i64_const_exp = I64Const::parse(in);
        if (!i64_const_exp)
            return Unexpected(PROPAGATE(i64_const_exp));
        return std::make_shared<I64Const>(*i64_const_exp);
    }
    case F32Const::OPCODE: {
        Expected<F32Const> f32_const_exp = F32Const::parse(in);
        if (!f32_const_exp)
            return Unexpected(PROPAGATE(f32_const_exp));
        return std::make_shared<F32Const>(*f32_const_exp);
    }
    case F64Const::OPCODE: {
        Expected<F64Const> f64_const_exp = F64Const::parse(in);
        if (!f64_const_exp)
            return Unexpected(PROPAGATE(f64_const_exp));
        return std::make_shared<F64Const>(*f64_const_exp);
    }
    case I32EqualZero::OPCODE:
        return std::make_shared<I32EqualZero>();
    case I32Equal::OPCODE:
        return std::make_shared<I32Equal>();
    case I32NotEqual::OPCODE:
        return std::make_shared<I32NotEqual>();
    case I32LessThanSigned::OPCODE:
        return std::make_shared<I32LessThanSigned>();
    case I32LessThanUnsigned::OPCODE:
        return std::make_shared<I32LessThanUnsigned>();
    case I32GreaterThanSigned::OPCODE:
        return std::make_shared<I32GreaterThanSigned>();
    case I32GreaterThanUnsigned::OPCODE:
        return std::make_shared<I32GreaterThanUnsigned>();
    case I32LessEqualSigned::OPCODE:
        return std::make_shared<I32LessEqualSigned>();
    case I32LessEqualUnsigned::OPCODE:
        return std::make_shared<I32LessEqualUnsigned>();
    case I32GreaterEqualSigned::OPCODE:
        return std::make_shared<I32GreaterEqualSigned>();
    case I32GreaterEqualUnsigned::OPCODE:
        return std::make_shared<I32GreaterEqualUnsigned>();
    case I64EqualZero::OPCODE:
        return std::make_shared<I64EqualZero>();
    case I64Equal::OPCODE:
        return std::make_shared<I64Equal>();
    case I64NotEqual::OPCODE:
        return std::make_shared<I64NotEqual>();
    case I64LessThanSigned::OPCODE:
        return std::make_shared<I64LessThanSigned>();
    case I64LessThanUnsigned::OPCODE:
        return std::make_shared<I64LessThanUnsigned>();
    case I64GreaterThanSigned::OPCODE:
        return std::make_shared<I64GreaterThanSigned>();
    case I64GreaterThanUnsigned::OPCODE:
        return std::make_shared<I64GreaterThanUnsigned>();
    case I64LessEqualSigned::OPCODE:
        return std::make_shared<I64LessEqualSigned>();
    case I64LessEqualUnsigned::OPCODE:
        return std::make_shared<I64LessEqualUnsigned>();
    case I64GreaterEqualSigned::OPCODE:
        return std::make_shared<I64GreaterEqualSigned>();
    case I64GreaterEqualUnsigned::OPCODE:
        return std::make_shared<I64GreaterEqualUnsigned>();
    case F32Equal::OPCODE:
        return std::make_shared<F32Equal>();
    case F32NotEqual::OPCODE:
        return std::make_shared<F32NotEqual>();
    case F32LessThan::OPCODE:
        return std::make_shared<F32LessThan>();
    case F32GreaterThan::OPCODE:
        return std::make_shared<F32GreaterThan>();
    case F32LessEqual::OPCODE:
        return std::make_shared<F32LessEqual>();
    case F32GreaterEqual::OPCODE:
        return std::make_shared<F32GreaterEqual>();
    case F64Equal::OPCODE:
        return std::make_shared<F64Equal>();
    case F64NotEqual::OPCODE:
        return std::make_shared<F64NotEqual>();
    case F64LessThan::OPCODE:
        return std::make_shared<F64LessThan>();
    case F64GreaterThan::OPCODE:
        return std::make_shared<F64GreaterThan>();
    case F64LessEqual::OPCODE:
        return std::make_shared<F64LessEqual>();
    case F64GreaterEqual::OPCODE:
        return std::make_shared<F64GreaterEqual>();
    case I32CountLeadingZeros::OPCODE:
        return std::make_shared<I32CountLeadingZeros>();
    case I32CountTrailingZeros::OPCODE:
        return std::make_shared<I32CountTrailingZeros>();
    case I32PopCount::OPCODE:
        return std::make_shared<I32PopCount>();
    case I32Add::OPCODE:
        return std::make_shared<I32Add>(addr);
    case I32Sub::OPCODE:
        return std::make_shared<I32Sub>(addr);
    case I32Mul::OPCODE:
        return std::make_shared<I32Mul>();
    case I32DivideSigned::OPCODE:
        return std::make_shared<I32DivideSigned>();
    case I32DivideUnsigned::OPCODE:
        return std::make_shared<I32DivideUnsigned>();
    case I32RemainderSigned::OPCODE:
        return std::make_shared<I32RemainderSigned>();
    case I32RemainderUnsigned::OPCODE:
        return std::make_shared<I32RemainderUnsigned>();
    case I32And::OPCODE:
        return std::make_shared<I32And>();
    case I32Or::OPCODE:
        return std::make_shared<I32Or>();
    case I32Xor::OPCODE:
        return std::make_shared<I32Xor>();
    case I32ShiftLeft::OPCODE:
        return std::make_shared<I32ShiftLeft>();
    case I32ShiftRightSigned::OPCODE:
        return std::make_shared<I32ShiftRightSigned>();
    case I32ShiftRightUnsigned::OPCODE:
        return std::make_shared<I32ShiftRightUnsigned>();
    case I32RotateLeft::OPCODE:
        return std::make_shared<I32RotateLeft>();
    case I32RotateRight::OPCODE:
        return std::make_shared<I32RotateRight>();
    case I64CountLeadingZeros::OPCODE:
        return std::make_shared<I64CountLeadingZeros>();
    case I64CountTrailingZeros::OPCODE:
        return std::make_shared<I64CountTrailingZeros>();
    case I64Add::OPCODE:
        return std::make_shared<I64Add>();
    case I64Sub::OPCODE:
        return std::make_shared<I64Sub>();
    case I64Mul::OPCODE:
        return std::make_shared<I64Mul>();
    case I64DivideSigned::OPCODE:
        return std::make_shared<I64DivideSigned>();
    case I64DivideUnsigned::OPCODE:
        return std::make_shared<I64DivideUnsigned>();
    case I64RemainderSigned::OPCODE:
        return std::make_shared<I64RemainderSigned>();
    case I64RemainderUnsigned::OPCODE:
        return std::make_shared<I64RemainderUnsigned>();
    case I64And::OPCODE:
        return std::make_shared<I64And>();
    case I64Or::OPCODE:
        return std::make_shared<I64Or>();
    case I64Xor::OPCODE:
        return std::make_shared<I64Xor>();
    case I64ShiftLeft::OPCODE:
        return std::make_shared<I64ShiftLeft>();
    case I64ShiftRightSigned::OPCODE:
        return std::make_shared<I64ShiftRightSigned>();
    case I64ShiftRightUnsigned::OPCODE:
        return std::make_shared<I64ShiftRightUnsigned>();
    case I64RotateLeft::OPCODE:
        return std::make_shared<I64RotateLeft>();
    case I64RotateRight::OPCODE:
        return std::make_shared<I64RotateRight>();
    case F32Mul::OPCODE:
        return std::make_shared<F32Mul>();
    case F32Div::OPCODE:
        return std::make_shared<F32Div>();
    case F64Neg::OPCODE:
        return std::make_shared<F64Neg>();
    case F64Add::OPCODE:
        return std::make_shared<F64Add>();
    case F64Sub::OPCODE:
        return std::make_shared<F64Sub>();
    case F64Mul::OPCODE:
        return std::make_shared<F64Mul>();
    case F64Div::OPCODE:
        return std::make_shared<F64Div>();
    case F64CopySign::OPCODE:
        return std::make_shared<F64CopySign>();
    case I32WrapI64::OPCODE:
        return std::make_shared<I32WrapI64>();
    case I64ExtendI32Signed::OPCODE:
        return std::make_shared<I64ExtendI32Signed>();
    case I64ExtendI32Unsigned::OPCODE:
        return std::make_shared<I64ExtendI32Unsigned>();
    case F32ConvertI32Signed::OPCODE:
        return std::make_shared<F32ConvertI32Signed>();
    case F32ConvertI32Unsigned::OPCODE:
        return std::make_shared<F32ConvertI32Unsigned>();
    case F32DemoteF64::OPCODE:
        return std::make_shared<F32DemoteF64>();
    case F64ConvertI32Signed::OPCODE:
        return std::make_shared<F64ConvertI32Signed>();
    case F64ConvertI32Unsigned::OPCODE:
        return std::make_shared<F64ConvertI32Unsigned>();
    case F64ConvertI64Signed::OPCODE:
        return std::make_shared<F64ConvertI64Signed>();
    case F64ConvertI64Unsigned::OPCODE:
        return std::make_shared<F64ConvertI64Unsigned>();
    case F64PromoteF32::OPCODE:
        return std::make_shared<F64PromoteF32>();
    case I32ReinterpretF32::OPCODE:
        return std::make_shared<I32ReinterpretF32>();
    case I64ReinterpretF64::OPCODE:
        return std::make_shared<I64ReinterpretF64>();
    case F32ReinterpretI32::OPCODE:
        return std::make_shared<F32ReinterpretI32>();
    case F64ReinterpretI64::OPCODE:
        return std::make_shared<F64ReinterpretI64>();
    case I32Extend8Signed::OPCODE:
        return std::make_shared<I32Extend8Signed>();
    case I32Extend16Signed::OPCODE:
        return std::make_shared<I32Extend16Signed>();
    case I64Extend8Signed::OPCODE:
        return std::make_shared<I64Extend8Signed>();
    case I64Extend16Signed::OPCODE:
        return std::make_shared<I64Extend16Signed>();
    case I64Extend32Signed::OPCODE:
        return std::make_shared<I64Extend32Signed>();
    case I32Load::OPCODE: {
        Expected<I32Load> i32_load_exp = I32Load::parse(in, addr);
        if (!i32_load_exp)
            return Unexpected(PROPAGATE(i32_load_exp));
        return std::make_shared<I32Load>(*i32_load_exp);
    }
    case I64Load::OPCODE: {
        Expected<I64Load> i64_load_exp = I64Load::parse(in);
        if (!i64_load_exp)
            return Unexpected(PROPAGATE(i64_load_exp));
        return std::make_shared<I64Load>(*i64_load_exp);
    }
    case F32Load::OPCODE: {
        Expected<F32Load> f32_load_exp = F32Load::parse(in);
        if (!f32_load_exp)
            return Unexpected(PROPAGATE(f32_load_exp));
        return std::make_shared<F32Load>(*f32_load_exp);
    }
    case F64Load::OPCODE: {
        Expected<F64Load> f64_load_exp = F64Load::parse(in);
        if (!f64_load_exp)
            return Unexpected(PROPAGATE(f64_load_exp));
        return std::make_shared<F64Load>(*f64_load_exp);
    }
    case I32Load8Signed::OPCODE: {
        Expected<I32Load8Signed> i32_load8_s_exp = I32Load8Signed::parse(in);
        if (!i32_load8_s_exp)
            return Unexpected(PROPAGATE(i32_load8_s_exp));
        return std::make_shared<I32Load8Signed>(*i32_load8_s_exp);
    }
    case I32Load8Unsigned::OPCODE: {
        Expected<I32Load8Unsigned> i32_load8_u_exp =
            I32Load8Unsigned::parse(in);
        if (!i32_load8_u_exp)
            return Unexpected(PROPAGATE(i32_load8_u_exp));
        return std::make_shared<I32Load8Unsigned>(*i32_load8_u_exp);
    }
    case I32Load16Signed::OPCODE: {
        Expected<I32Load16Signed> i32_load16_s_exp = I32Load16Signed::parse(in);
        if (!i32_load16_s_exp)
            return Unexpected(PROPAGATE(i32_load16_s_exp));
        return std::make_shared<I32Load16Signed>(*i32_load16_s_exp);
    }
    case I32Load16Unsigned::OPCODE: {
        Expected<I32Load16Unsigned> i32_load16_u_exp =
            I32Load16Unsigned::parse(in);
        if (!i32_load16_u_exp)
            return Unexpected(PROPAGATE(i32_load16_u_exp));
        return std::make_shared<I32Load16Unsigned>(*i32_load16_u_exp);
    }
    case I64Load8Signed::OPCODE: {
        Expected<I64Load8Signed> i64_load8_s_exp = I64Load8Signed::parse(in);
        if (!i64_load8_s_exp)
            return Unexpected(PROPAGATE(i64_load8_s_exp));
        return std::make_shared<I64Load8Signed>(*i64_load8_s_exp);
    }
    case I64Load8Unsigned::OPCODE: {
        Expected<I64Load8Unsigned> i64_load8_u_exp =
            I64Load8Unsigned::parse(in);
        if (!i64_load8_u_exp)
            return Unexpected(PROPAGATE(i64_load8_u_exp));
        return std::make_shared<I64Load8Unsigned>(*i64_load8_u_exp);
    }
    case I64Load16Signed::OPCODE: {
        Expected<I64Load16Signed> i64_load16_s_exp = I64Load16Signed::parse(in);
        if (!i64_load16_s_exp)
            return Unexpected(PROPAGATE(i64_load16_s_exp));
        return std::make_shared<I64Load16Signed>(*i64_load16_s_exp);
    }
    case I64Load16Unsigned::OPCODE: {
        Expected<I64Load16Unsigned> i64_load16_u_exp =
            I64Load16Unsigned::parse(in);
        if (!i64_load16_u_exp)
            return Unexpected(PROPAGATE(i64_load16_u_exp));
        return std::make_shared<I64Load16Unsigned>(*i64_load16_u_exp);
    }
    case I64Load32Signed::OPCODE: {
        Expected<I64Load32Signed> i64_load32_s_exp = I64Load32Signed::parse(in);
        if (!i64_load32_s_exp)
            return Unexpected(PROPAGATE(i64_load32_s_exp));
        return std::make_shared<I64Load32Signed>(*i64_load32_s_exp);
    }
    case I64Load32Unsigned::OPCODE: {
        Expected<I64Load32Unsigned> i64_load32_u_exp =
            I64Load32Unsigned::parse(in);
        if (!i64_load32_u_exp)
            return Unexpected(PROPAGATE(i64_load32_u_exp));
        return std::make_shared<I64Load32Unsigned>(*i64_load32_u_exp);
    }
    case I32Store::OPCODE: {
        Expected<I32Store> i32_store_exp = I32Store::parse(in, addr);
        if (!i32_store_exp)
            return Unexpected(PROPAGATE(i32_store_exp));
        return std::make_shared<I32Store>(*i32_store_exp);
    }
    case I64Store::OPCODE: {
        Expected<I64Store> i64_store_exp = I64Store::parse(in);
        if (!i64_store_exp)
            return Unexpected(PROPAGATE(i64_store_exp));
        return std::make_shared<I64Store>(*i64_store_exp);
    }
    case F32Store::OPCODE: {
        Expected<F32Store> f32_store_exp = F32Store::parse(in);
        if (!f32_store_exp)
            return Unexpected(PROPAGATE(f32_store_exp));
        return std::make_shared<F32Store>(*f32_store_exp);
    }
    case F64Store::OPCODE: {
        Expected<F64Store> f64_store_exp = F64Store::parse(in);
        if (!f64_store_exp)
            return Unexpected(PROPAGATE(f64_store_exp));
        return std::make_shared<F64Store>(*f64_store_exp);
    }
    case I32Store8::OPCODE: {
        Expected<I32Store8> i32_store8_exp = I32Store8::parse(in);
        if (!i32_store8_exp)
            return Unexpected(PROPAGATE(i32_store8_exp));
        return std::make_shared<I32Store8>(*i32_store8_exp);
    }
    case I32Store16::OPCODE: {
        Expected<I32Store16> i32_store16_exp = I32Store16::parse(in);
        if (!i32_store16_exp)
            return Unexpected(PROPAGATE(i32_store16_exp));
        return std::make_shared<I32Store16>(*i32_store16_exp);
    }
    case I64Store8::OPCODE: {
        Expected<I64Store8> i64_store8_exp = I64Store8::parse(in);
        if (!i64_store8_exp)
            return Unexpected(PROPAGATE(i64_store8_exp));
        return std::make_shared<I64Store8>(*i64_store8_exp);
    }
    case I64Store16::OPCODE: {
        Expected<I64Store16> i64_store16_exp = I64Store16::parse(in);
        if (!i64_store16_exp)
            return Unexpected(PROPAGATE(i64_store16_exp));
        return std::make_shared<I64Store16>(*i64_store16_exp);
    }
    case I64Store32::OPCODE: {
        Expected<I64Store32> i64_store32_exp = I64Store32::parse(in);
        if (!i64_store32_exp)
            return Unexpected(PROPAGATE(i64_store32_exp));
        return std::make_shared<I64Store32>(*i64_store32_exp);
    }
    case MemoryGrow::OPCODE: {
        Expected<MemoryGrow> memory_grow_exp = MemoryGrow::parse(in, addr);
        if (!memory_grow_exp)
            return Unexpected(PROPAGATE(memory_grow_exp));
        return std::make_shared<MemoryGrow>(*memory_grow_exp);
    }
    case ExtendedIntstructionBase::OPCODE:
        return ExtendedIntstructionBase::parse(in, addr);
    case AtomicIntstructionBase::OPCODE:
        return AtomicIntstructionBase::parse(in);
    default:
        return Unexpected(ERROR(fmt::format("unknown opcode: {:02X}", opcode)));
    }
}

Expected<Instruction> ExtendedIntstructionBase::parse(ByteCursor& in,
                                                      size_t addr) {
    uint8_t xop = in.byte();

    switch (xop) {
    case I32TruncateSaturateF64Signed::EXT_OPCODE:
        return std::make_shared<I32TruncateSaturateF64Signed>(addr);
    case I32TruncateSaturateF64Unsigned::EXT_OPCODE:
        return std::make_shared<I32TruncateSaturateF64Unsigned>(addr);
    case I64TruncateSaturateF64Signed::EXT_OPCODE:
        return std::make_shared<I64TruncateSaturateF64Signed>(addr);
    case MemoryInit::EXT_OPCODE: {
        Expected<MemoryInit> mem_init_exp = MemoryInit::parse(in);
        if (!mem_init_exp)
            return Unexpected(PROPAGATE(mem_init_exp));
        return std::make_shared<MemoryInit>(*mem_init_exp);
    }
    case DataDrop::EXT_OPCODE: {
        Expected<DataDrop> data_drop_exp = DataDrop::parse(in);
        if (!data_drop_exp)
            return Unexpected(PROPAGATE(data_drop_exp));
        return std::make_shared<DataDrop>(*data_drop_exp);
    }
    case MemoryCopy::EXT_OPCODE: {
        Expected<MemoryCopy> mem_copy_exp = MemoryCopy::parse(in);
        if (!mem_copy_exp)
            return Unexpected(PROPAGATE(mem_copy_exp));
        return std::make_shared<MemoryCopy>(*mem_copy_exp);
    }
    case MemoryFill::EXT_OPCODE: {
        Expected<MemoryFill> mem_fill_exp = MemoryFill::parse(in, addr);
        if (!mem_fill_exp)
            return Unexpected(PROPAGATE(mem_fill_exp));
        return std::make_shared<MemoryFill>(*mem_fill_exp);
    }
    default:
        return Unexpected(ERROR(
            fmt::format("unknown extended instruction opcode: {:02X} {:02X}",
                        ExtendedIntstructionBase::OPCODE, xop)));
    }
}

/************************/
/* Control Instructions */
/************************/

Expected<BlockType> BlockType::parse(ByteCursor& in) {
    uint8_t byte = in.peeek();
    
    if (byte == 0x40) {
        /* consume byte */
        in.byte();
        return BlockType({});
    }

    Expected<ValueType> val_type_exp = ValueType::parse(in);
    if (!val_type_exp)
        return Unexpected(PROPAGATE(val_type_exp));

    return BlockType(*val_type_exp);
}

std::string BlockType::toString() const {
    return val_type_opt_.has_value() ? val_type_opt_->toString() : "";
}

Expected<Block> Block::parse(ByteCursor& in, size_t code_start) {
    Expected<BlockType> block_type_exp = BlockType::parse(in);
    if (!block_type_exp)
        return Unexpected(PROPAGATE(block_type_exp));

    std::vector<Instruction> instructions;

    while (true) {
        Expected<Instruction> inst_exp = InstructionBase::parse(in, code_start);
        if (!inst_exp)
            return Unexpected(PROPAGATE(inst_exp));
        const Instruction& inst = *inst_exp;

        if (inst->is<End>())
            break;

        instructions.push_back(inst);
    }

    return Block(*block_type_exp, std::move(instructions));
}

std::string Block::toString() const {
    std::vector<std::string> fmt_instructions;
    fmt_instructions.reserve(instructions_.size());

    for (size_t i = 0; i < instructions_.size(); i++)
        fmt_instructions.push_back(instructions_[i]->toString());

    return fmt::format("block\n{}\nend", fmt::join(fmt_instructions, "\n"));
}

Expected<Loop> Loop::parse(ByteCursor& in, size_t addr, size_t code_start) {
    Expected<BlockType> block_type_exp = BlockType::parse(in);
    if (!block_type_exp)
        return Unexpected(PROPAGATE(block_type_exp));

    std::vector<Instruction> instructions;

    while (true) {
        Expected<Instruction> inst_exp = InstructionBase::parse(in, code_start);
        if (!inst_exp)
            return Unexpected(PROPAGATE(inst_exp));
        const Instruction& inst = *inst_exp;

        if (inst->is<End>())
            break;

        instructions.push_back(inst);
    }

    return Loop(*block_type_exp, std::move(instructions), addr);
}

std::string Loop::toString() const {
    std::vector<std::string> fmt_instructions;
    fmt_instructions.reserve(instructions_.size());

    for (size_t i = 0; i < instructions_.size(); i++)
        fmt_instructions.push_back(instructions_[i]->toString());

    return fmt::format("loop\n{}\nend", fmt::join(fmt_instructions, "\n"));
}

Expected<IfElse> IfElse::parse(ByteCursor& in, size_t addr,
                               size_t code_start) {
    Expected<BlockType> block_type_exp = BlockType::parse(in);
    if (!block_type_exp)
        return Unexpected(PROPAGATE(block_type_exp));

    Expected<Expression> then_expr_exp = Expression::parse(in, code_start);
    if (!then_expr_exp)
        return Unexpected(PROPAGATE(then_expr_exp));

    Expression& then_expr = *then_expr_exp;
    if (then_expr.getTerminator()->is<End>())
        return IfElse(std::move(*then_expr_exp), addr);

    assert(then_expr.getTerminator()->is<Else>());

    Expected<Expression> else_expr_exp = Expression::parse(in, code_start);
    if (!else_expr_exp)
        return Unexpected(PROPAGATE(else_expr_exp));

    return IfElse(std::move(then_expr), std::move(*else_expr_exp), addr);
}

std::string IfElse::toString() const {
    return fmt::format("if\n{}\n{}end", then_expr_.toString(),
                       hasElseExpression()
                           ? fmt::format("else\n{}\n", else_expr_->toString())
                           : "");
}

Expected<Branch> Branch::parse(ByteCursor& in) {
    Expected<U32> label_idx_exp = U32::parse(in);
    if (!label_idx_exp)
        return Unexpected(PROPAGATE(label_idx_exp));
    uint32_t label_idx = *label_idx_exp;

    return Branch(label_idx);
}

std::string Branch::toString() const {
    return fmt::format("br {}", label_idx_);
}

Expected<BranchIf> BranchIf::parse(ByteCursor& in, size_t addr) {
    Expected<U32> label_idx_exp = U32::parse(in);
    if (!label_idx_exp)
        return Unexpected(PROPAGATE(label_idx_exp));
    uint32_t label_idx = *label_idx_exp;

    return BranchIf(label_idx, addr);
}

std::string BranchIf::toString() const {
    return fmt::format("br_if {}", label_idx_);
}

Expected<BranchTable> BranchTable::parse(ByteCursor& in) {
    Expected<U32> len_exp = U32::parse(in);
    if (!len_exp)
        return Unexpected(PROPAGATE(len_exp));
    uint32_t len = static_cast<uint32_t>(*len_exp) + 1;

    std::vector<uint32_t> label_indices;
    label_indices.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<U32> label_idx_exp = U32::parse(in);
        if (!label_idx_exp)
            return Unexpected(PROPAGATE(label_idx_exp));
        uint32_t label_idx = *label_idx_exp;
        label_indices.push_back(label_idx);
    }

    return BranchTable(std::move(label_indices));
}

std::string BranchTable::toString() const {
    return fmt::format("br_table {}", fmt::join(label_indices_, " "));
}

Expected<Call> Call::parse(ByteCursor& in, size_t addr) {
    Expected<U32> func_idx_exp = U32::parse(in);
    if (!func_idx_exp)
        return Unexpected(PROPAGATE(func_idx_exp));
    uint32_t func_idx = *func_idx_exp;

    return Call(func_idx, addr);
}

std::string Call::toString() const { return fmt::format("call {}", func_idx_); }

Expected<CallIndirect> CallIndirect::parse(ByteCursor& in, size_t addr) {
    Expected<U32> type_idx_exp = U32::parse(in);
    if (!type_idx_exp)
        return Unexpected(PROPAGATE(type_idx_exp));
    uint32_t type_idx = *type_idx_exp;

    Expected<U32> table_idx_exp = U32::parse(in);
    if (!table_idx_exp)
        return Unexpected(PROPAGATE(table_idx_exp));
    uint32_t table_idx = *table_idx_exp;

    return CallIndirect(type_idx, table_idx, addr);
}

std::string CallIndirect::toString() const {
    return fmt::format("call_indirect {} {}", type_idx_, table_idx_);
}

/*************************/
/* Variable Instructions */
/*************************/

Expected<LocalGet> LocalGet::parse(ByteCursor& in, size_t addr) {
    Expected<U32> local_idx_exp = U32::parse(in);
    if (!local_idx_exp)
        return Unexpected(PROPAGATE(local_idx_exp));
    uint32_t local_idx = *local_idx_exp;

    return LocalGet(local_idx, addr);
}

std::string LocalGet::toString() const {
    return fmt::format("local.get {}", local_idx_);
}

Expected<LocalSet> LocalSet::parse(ByteCursor& in, size_t addr) {
    Expected<U32> local_idx_exp = U32::parse(in);
    if (!local_idx_exp)
        return Unexpected(PROPAGATE(local_idx_exp));
    uint32_t local_idx = *local_idx_exp;

    return LocalSet(local_idx, addr);
}

std::string LocalSet::toString() const {
    return fmt::format("local.set {}", local_idx_);
}

Expected<LocalTee> LocalTee::parse(ByteCursor& in, size_t addr) {
    Expected<U32> local_idx_exp = U32::parse(in);
    if (!local_idx_exp)
        return Unexpected(PROPAGATE(local_idx_exp));
    return LocalTee(*local_idx_exp, addr);
}

std::string LocalTee::toString() const {
    return fmt::format("local.tee {}", local_idx_);
}

Expected<GlobalGet> GlobalGet::parse(ByteCursor& in) {
    Expected<U32> global_idx_exp = U32::parse(in);
    if (!global_idx_exp)
        return Unexpected(PROPAGATE(global_idx_exp));
    uint32_t global_idx = *global_idx_exp;

    return GlobalGet(global_idx);
}

std::string GlobalGet::toString() const {
    return fmt::format("global.get {}", global_idx_);
}

Expected<GlobalSet> GlobalSet::parse(ByteCursor& in) {
    Expected<U32> global_idx_exp = U32::parse(in);
    if (!global_idx_exp)
        return Unexpected(PROPAGATE(global_idx_exp));
    uint32_t global_idx = *global_idx_exp;

    return GlobalSet(global_idx);
}

std::string GlobalSet::toString() const {
    return fmt::format("global.set {}", global_idx_);
}

/************************/
/* Numeric Instructions */
/************************/

Expected<I32Const> I32Const::parse(ByteCursor& in, size_t addr) {
    Expected<S32> i32_exp = S32::parse(in);
    if (!i32_exp)
        return Unexpected(PROPAGATE(i32_exp));
    int32_t i32 = *i32_exp;

    return I32Const(i32, addr);
}

std::string I32Const::toString() const {
    return fmt::format("i32.const {}", i_);
}

Expected<I64Const> I64Const::parse(ByteCursor& in) {
    Expected<S64> i64_exp = S64::parse(in);
    if (!i64_exp)
        return Unexpected(PROPAGATE(i64_exp));
    int64_t i32 = *i64_exp;

    return I64Const(i32);
}

std::string I64Const::toString() const {
    return fmt::format("i64.const {}", i_);
}

Expected<F32Const> F32Const::parse(ByteCursor& in) {
    Expected<F32> f32_exp = F32::parse(in);
    if (!f32_exp)
        return Unexpected(PROPAGATE(f32_exp));

    return F32Const(std::move(*f32_exp));
}

std::string F32Const::toString() const {
    return fmt::format("f32.const {}", getVal());
}

Expected<F64Const> F64Const::parse(ByteCursor& in) {
    Expected<F64> f64_exp = F64::parse(in);
    if (!f64_exp)
        return Unexpected(PROPAGATE(f64_exp));

    return F64Const(std::move(*f64_exp));
}

std::string F64Const::toString() const {
    return fmt::format("f64.const {}", getVal());
}

/***********************/
/* Memory Instructions */
/***********************/

Expected<MemoryArgument> MemoryArgument::parse(ByteCursor& in) {
    Expected<U32> align_exp = U32::parse(in);
    if (!align_exp)
        return Unexpected(PROPAGATE(align_exp));

    Expected<U32> offset_exp = U32::parse(in);
    if (!offset_exp)
        return Unexpected(PROPAGATE(offset_exp));

    return MemoryArgument(1u << *align_exp, *offset_exp);
}

std::string MemoryArgument::toString() const {
    return fmt::format("{} {}", offset_, align_);
}

Expected<I32Load> I32Load::parse(ByteCursor& in, size_t addr) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I32Load(*mem_arg_exp, addr);
}

std::string I32Load::toString() const {
    return fmt::format("i32.load {}", mem_arg_.toString());
}

Expected<I64Load> I64Load::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Load(*mem_arg_exp);
}

std::string I64Load::toString() const {
    return fmt::format("i64.load {}", mem_arg_.toString());
}

Expected<F32Load> F32Load::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return F32Load(*mem_arg_exp);
}

std::string F32Load::toString() const {
    return fmt::format("f32.load {}", mem_arg_.toString());
}

Expected<F64Load> F64Load::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return F64Load(*mem_arg_exp);
}

std::string F64Load::toString() const {
    return fmt::format("f64.load {}", mem_arg_.toString());
}

Expected<I32Load8Signed> I32Load8Signed::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I32Load8Signed(*mem_arg_exp);
}

std::string I32Load8Signed::toString() const {
    return fmt::format("i32.load8_s {}", mem_arg_.toString());
}

Expected<I32Load8Unsigned> I32Load8Unsigned::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I32Load8Unsigned(*mem_arg_exp);
}

std::string I32Load8Unsigned::toString() const {
    return fmt::format("i32.load8_u {}", mem_arg_.toString());
}

Expected<I32Load16Signed> I32Load16Signed::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I32Load16Signed(*mem_arg_exp);
}

std::string I32Load16Signed::toString() const {
    return fmt::format("i32.load16_s {}", mem_arg_.toString());
}

Expected<I32Load16Unsigned> I32Load16Unsigned::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I32Load16Unsigned(*mem_arg_exp);
}

std::string I32Load16Unsigned::toString() const {
    return fmt::format("i32.load16_u {}", mem_arg_.toString());
}

Expected<I64Load8Signed> I64Load8Signed::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Load8Signed(*mem_arg_exp);
}

std::string I64Load8Signed::toString() const {
    return fmt::format("i64.load8_s {}", MemoryArgument::toString());
}

Expected<I64Load8Unsigned> I64Load8Unsigned::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Load8Unsigned(*mem_arg_exp);
}

std::string I64Load8Unsigned::toString() const {
    return fmt::format("i64.load8_u {}", MemoryArgument::toString());
}

Expected<I64Load16Signed> I64Load16Signed::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Load16Signed(*mem_arg_exp);
}

std::string I64Load16Signed::toString() const {
    return fmt::format("i64.load16_s {}", MemoryArgument::toString());
}

Expected<I64Load16Unsigned> I64Load16Unsigned::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Load16Unsigned(*mem_arg_exp);
}

std::string I64Load16Unsigned::toString() const {
    return fmt::format("i64.load16_u {}", MemoryArgument::toString());
}

Expected<I64Load32Signed> I64Load32Signed::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Load32Signed(*mem_arg_exp);
}

std::string I64Load32Signed::toString() const {
    return fmt::format("i64.load32_s {}", MemoryArgument::toString());
}

Expected<I64Load32Unsigned> I64Load32Unsigned::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Load32Unsigned(*mem_arg_exp);
}

std::string I64Load32Unsigned::toString() const {
    return fmt::format("i64.load32_u {}", MemoryArgument::toString());
}

Expected<I32Store> I32Store::parse(ByteCursor& in, size_t addr) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I32Store(*mem_arg_exp, addr);
}

std::string I32Store::toString() const {
    return fmt::format("i32.store {}", mem_arg_.toString());
}

Expected<I64Store> I64Store::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Store(*mem_arg_exp);
}

std::string I64Store::toString() const {
    return fmt::format("i64.store {}", mem_arg_.toString());
}

Expected<F32Store> F32Store::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return F32Store(*mem_arg_exp);
}

std::string F32Store::toString() const {
    return fmt::format("f32.store {}", MemoryArgument::toString());
}

Expected<F64Store> F64Store::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return F64Store(*mem_arg_exp);
}

std::string F64Store::toString() const {
    return fmt::format("f64.store {}", MemoryArgument::toString());
}

Expected<I32Store8> I32Store8::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I32Store8(*mem_arg_exp);
}

std::string I32Store8::toString() const {
    return fmt::format("i32.store8 {}", mem_arg_.toString());
}

Expected<I32Store16> I32Store16::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I32Store16(*mem_arg_exp);
}

std::string I32Store16::toString() const {
    return fmt::format("i32.store16 {}", mem_arg_.toString());
}

Expected<I64Store8> I64Store8::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Store8(*mem_arg_exp);
}

std::string I64Store8::toString() const {
    return fmt::format("i64.store8 {}", mem_arg_.toString());
}

Expected<I64Store16> I64Store16::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Store16(*mem_arg_exp);
}

std::string I64Store16::toString() const {
    return fmt::format("i64.store16 {}", mem_arg_.toString());
}

Expected<I64Store32> I64Store32::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return I64Store32(*mem_arg_exp);
}

std::string I64Store32::toString() const {
    return fmt::format("i64.store32 {}", mem_arg_.toString());
}

Expected<MemoryGrow> MemoryGrow::parse(ByteCursor& in, size_t addr) {
    uint8_t mem_idx = in.byte();
    if (mem_idx != 0)
        return Unexpected(ERROR("multi memory currently not supported"));

    return MemoryGrow(addr);
}

std::string MemoryGrow::toString() const { return "memory.grow 0"; }

Expected<MemoryInit> MemoryInit::parse(ByteCursor& in) {
    Expected<U32> segment_idx_exp = U32::parse(in);
    if (!segment_idx_exp)
        return Unexpected(PROPAGATE(segment_idx_exp));
    uint32_t segment_idx = *segment_idx_exp;

    uint8_t zero = in.byte();
    if (zero != 0)
        return Unexpected(ERROR("invalid trailing bytes"));

    return MemoryInit(segment_idx);
}

std::string MemoryInit::toString() const {
    return fmt::format("memory.init {}", segment_idx_);
}

Expected<DataDrop> DataDrop::parse(ByteCursor& in) {
    Expected<U32> segment_idx_exp = U32::parse(in);
    if (!segment_idx_exp)
        return Unexpected(PROPAGATE(segment_idx_exp));
    uint32_t segment_idx = *segment_idx_exp;

    return DataDrop(segment_idx);
}

std::string DataDrop::toString() const {
    return fmt::format("data.drop {}", segment_idx_);
}

Expected<MemoryCopy> MemoryCopy::parse(ByteCursor& in) {
    uint16_t zero = in.u16_le();
    if (zero != 0)
        return Unexpected(ERROR("invalid trailing bytes"));
    
    return MemoryCopy();
}

Expected<MemoryFill> MemoryFill::parse(ByteCursor& in, size_t addr) {
    uint8_t zero = in.byte();
    if (zero != 0)
        return Unexpected(ERROR("invalid trailing bytes"));

    return MemoryFill(addr);
}

/******************************/
/* Atomic Memory Instructions */
/******************************/

Expected<Instruction> AtomicIntstructionBase::parse(ByteCursor& in) {
    uint8_t atomic_opcode = in.byte();

    switch (atomic_opcode) {
    case AtomicNotify::ATOMIC_OPCODE: {
        Expected<AtomicNotify> atomic_notify_exp = AtomicNotify::parse(in);
        if (!atomic_notify_exp)
            return Unexpected(PROPAGATE(atomic_notify_exp));
        return std::make_shared<AtomicNotify>(*atomic_notify_exp);
    }
    case AtomicWait32::ATOMIC_OPCODE: {
        Expected<AtomicWait32> atomic_wait_32_exp = AtomicWait32::parse(in);
        if (!atomic_wait_32_exp)
            return Unexpected(PROPAGATE(atomic_wait_32_exp));
        return std::make_shared<AtomicWait32>(*atomic_wait_32_exp);
    }
    case AtomicLoad::ATOMIC_OPCODE: {
        Expected<AtomicLoad> atomic_load_exp = AtomicLoad::parse(in);
        if (!atomic_load_exp)
            return Unexpected(PROPAGATE(atomic_load_exp));
        return std::make_shared<AtomicLoad>(*atomic_load_exp);
    }
    case AtomicLoad8Unsigned::ATOMIC_OPCODE: {
        Expected<AtomicLoad8Unsigned> atomic_load8_u_exp =
            AtomicLoad8Unsigned::parse(in);
        if (!atomic_load8_u_exp)
            return Unexpected(PROPAGATE(atomic_load8_u_exp));
        return std::make_shared<AtomicLoad8Unsigned>(*atomic_load8_u_exp);
    }
    case AtomicStore::ATOMIC_OPCODE: {
        Expected<AtomicStore> atomic_store_exp = AtomicStore::parse(in);
        if (!atomic_store_exp)
            return Unexpected(PROPAGATE(atomic_store_exp));
        return std::make_shared<AtomicStore>(*atomic_store_exp);
    }
    case AtomicStore8::ATOMIC_OPCODE: {
        Expected<AtomicStore8> atomic_store8_exp = AtomicStore8::parse(in);
        if (!atomic_store8_exp)
            return Unexpected(PROPAGATE(atomic_store8_exp));
        return std::make_shared<AtomicStore8>(*atomic_store8_exp);
    }
    case AtomicAdd::ATOMIC_OPCODE: {
        Expected<AtomicAdd> atomic_add_exp = AtomicAdd::parse(in);
        if (!atomic_add_exp)
            return Unexpected(PROPAGATE(atomic_add_exp));
        return std::make_shared<AtomicAdd>(*atomic_add_exp);
    }
    case AtomicExchange8Unsigned::ATOMIC_OPCODE: {
        Expected<AtomicExchange8Unsigned> atomic_xchg8_u_exp =
            AtomicExchange8Unsigned::parse(in);
        if (!atomic_xchg8_u_exp)
            return Unexpected(PROPAGATE(atomic_xchg8_u_exp));
        return std::make_shared<AtomicExchange8Unsigned>(*atomic_xchg8_u_exp);
    }
    case AtomicCompareExchange::ATOMIC_OPCODE: {
        Expected<AtomicCompareExchange> atomic_cmpxchg_exp =
            AtomicCompareExchange::parse(in);
        if (!atomic_cmpxchg_exp)
            return Unexpected(PROPAGATE(atomic_cmpxchg_exp));
        return std::make_shared<AtomicCompareExchange>(*atomic_cmpxchg_exp);
    }
    case AtomicCompareExchange8Unsigned::ATOMIC_OPCODE: {
        Expected<AtomicCompareExchange8Unsigned> atomic_cmpxchg8_u_exp =
            AtomicCompareExchange8Unsigned::parse(in);
        if (!atomic_cmpxchg8_u_exp)
            return Unexpected(PROPAGATE(atomic_cmpxchg8_u_exp));
        return std::make_shared<AtomicCompareExchange8Unsigned>(
            *atomic_cmpxchg8_u_exp);
    }
    default:
        return Unexpected(ERROR(
            fmt::format("unknown atomic instruction opcode: {:02X} {:02X}",
                        AtomicIntstructionBase::OPCODE, atomic_opcode)));
    }
}

Expected<AtomicNotify> AtomicNotify::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return AtomicNotify(*mem_arg_exp);
}

std::string AtomicNotify::toString() const {
    return fmt::format("memory.atomic.notify {}", mem_arg_.toString());
}

Expected<AtomicWait32> AtomicWait32::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return AtomicWait32(*mem_arg_exp);
}

std::string AtomicWait32::toString() const {
    return fmt::format("memory.atomic.wait32 {}", mem_arg_.toString());
}

Expected<AtomicLoad> AtomicLoad::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return AtomicLoad(*mem_arg_exp);
}

std::string AtomicLoad::toString() const {
    return fmt::format("i32.atomic.load {}", mem_arg_.toString());
}

Expected<AtomicLoad8Unsigned> AtomicLoad8Unsigned::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return AtomicLoad8Unsigned(*mem_arg_exp);
}

std::string AtomicLoad8Unsigned::toString() const {
    return fmt::format("i32.atomic.load8_u {}", mem_arg_.toString());
}

Expected<AtomicStore> AtomicStore::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return AtomicStore(*mem_arg_exp);
}

std::string AtomicStore::toString() const {
    return fmt::format("i32.atomic.store {}", mem_arg_.toString());
}

Expected<AtomicStore8> AtomicStore8::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return AtomicStore8(*mem_arg_exp);
}

std::string AtomicStore8::toString() const {
    return fmt::format("i32.atomic.store8 {}", mem_arg_.toString());
}

Expected<AtomicAdd> AtomicAdd::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return AtomicAdd(*mem_arg_exp);
}

std::string AtomicAdd::toString() const {
    return fmt::format("i32.atomic.rmw.add {}", mem_arg_.toString());
}

Expected<AtomicExchange8Unsigned>
AtomicExchange8Unsigned::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return AtomicExchange8Unsigned(*mem_arg_exp);
}

std::string AtomicExchange8Unsigned::toString() const {
    return fmt::format("i32.atomic.rmw8.xchg_u {}", mem_arg_.toString());
}

Expected<AtomicCompareExchange> AtomicCompareExchange::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return AtomicCompareExchange(*mem_arg_exp);
}

std::string AtomicCompareExchange::toString() const {
    return fmt::format("i32.atomic.rmw.cmpxchg {}", mem_arg_.toString());
}

Expected<AtomicCompareExchange8Unsigned>
AtomicCompareExchange8Unsigned::parse(ByteCursor& in) {
    Expected<MemoryArgument> mem_arg_exp = MemoryArgument::parse(in);
    if (!mem_arg_exp)
        return Unexpected(PROPAGATE(mem_arg_exp));

    return AtomicCompareExchange8Unsigned(*mem_arg_exp);
}

std::string AtomicCompareExchange8Unsigned::toString() const {
    return fmt::format("i32.atomic.rmw8.cmpxchg_u {}", mem_arg_.toString());
}

} // namespace grammar
