#include <fmt/base.h>

#include "hw/mem/mmu.hpp"
#include "runtime/context.hpp"
#include "runtime/instance.hpp"
#include "runtime/kernel.hpp"
#include "runtime/operations/operations_control.hpp"
#include "runtime/operations/operations_numeric.hpp"
#include "runtime/operations/operations_numeric_impl.hpp"
#include "runtime/operations/operations_variable.hpp"
#include "runtime/operations/operations_variable_impl.hpp"
#include "runtime/optimization.hpp"
#include "runtime/vm.hpp"

namespace runtime {

using hw::mem::VIRT_MEMORY;

Operation
OperationBase::create(const std::vector<grammar::Instruction>& instructions,
                      const std::vector<FunctionType>& func_types,
                      Operations& targets,
                      std::pmr::polymorphic_allocator<std::byte>& arena) {
    std::vector<std::tuple<Collector, size_t>> collectors;
    collectors.emplace_back(std::make_unique<CollectorBase>(instructions), 0);

    while (true) {
        auto& [collector, i] = collectors.back();
        const std::vector<grammar::Instruction>& instructions =
            collector->getInstructions();

        bool entered_block = false;

        for (; i < instructions.size(); i++) {
            const auto& inst = instructions[i];

            if (inst->is<grammar::Block>()) {
                i++;
                collectors.emplace_back(
                    Block::collect(inst->as<grammar::Block>(), targets, arena),
                    0);
                entered_block = true;
                break;
            }

            Operation next = do_create(inst, func_types, targets, arena);
            collector->add(next);
        }

        if (entered_block)
            continue;

        Operation result = std::get<0>(collectors.back())->finish();
        collectors.pop_back();

        if (collectors.size() == 0)
            return result;

        std::get<0>(collectors.back())->add(result);
    }
}

Operation
OperationBase::do_create(const grammar::Instruction& inst,
                         const std::vector<FunctionType>& func_types,
                         Operations& targets,
                         std::pmr::polymorphic_allocator<std::byte>& arena) {
    uint8_t opcode = inst->getOpcode();

    switch (opcode) {
    // Control Instructions
    case grammar::Unreachable::OPCODE:
        return arena_new<Unreachable>(arena, inst->as<grammar::Unreachable>());
    case grammar::Nop::OPCODE:
        return arena_new<Nop>(arena, inst->as<grammar::Nop>());
    /* case grammar::Block::OPCODE:
        return arena_new<Block>(arena, inst->as<grammar::Block>(),
                                            func_types, targets, arena); */
    case grammar::Loop::OPCODE:
        return arena_new<Loop>(arena, inst->as<grammar::Loop>(), func_types,
                               targets, arena);
        break;
    case grammar::IfElse::OPCODE: {
        grammar::IfElse if_else_inst = inst->as<grammar::IfElse>();

        if (if_else_inst.hasElseExpression())
            return arena_new<IfElse>(arena, if_else_inst, func_types, targets,
                                     arena);
        else
            return arena_new<IfThen>(arena, if_else_inst, func_types, targets,
                                     arena);

        break;
    }
    case grammar::Branch::OPCODE:
        return arena_new<Branch>(arena, inst->as<grammar::Branch>(), targets);
        break;
    case grammar::BranchIf::OPCODE:
        return arena_new<BranchIf>(arena, inst->as<grammar::BranchIf>(),
                                   targets);
        break;
    case grammar::BranchTable::OPCODE:
        return arena_new<BranchTable>(arena, inst->as<grammar::BranchTable>(),
                                      targets);
        break;
    case grammar::Call::OPCODE:
        return arena_new<Call>(arena, inst->as<grammar::Call>());
        break;
    case grammar::CallIndirect::OPCODE:
        return arena_new<CallIndirect>(arena, inst->as<grammar::CallIndirect>(),
                                       func_types);
        break;
    case grammar::Return::OPCODE:
        return arena_new<Return>(arena);
        break;
    // Parametric Instructions
    case grammar::Drop::OPCODE:
        return arena_new<Drop>(arena, inst->as<grammar::Drop>());
        break;
    case grammar::Select::OPCODE:
        return arena_new<Select>(arena);
        break;
    // Variable Instruction
    case grammar::LocalGet::OPCODE:
        return arena_new<LocalGet>(arena, inst->as<grammar::LocalGet>());
        break;
    case grammar::LocalSet::OPCODE:
        return arena_new<LocalSet>(arena, inst->as<grammar::LocalSet>());
        break;
    case grammar::LocalTee::OPCODE:
        return arena_new<LocalTee>(arena, inst->as<grammar::LocalTee>());
        break;
    case grammar::GlobalGet::OPCODE:
        return arena_new<GlobalGet>(arena, inst->as<grammar::GlobalGet>());
        break;
    case grammar::GlobalSet::OPCODE:
        return arena_new<GlobalSet>(arena, inst->as<grammar::GlobalSet>());
        break;
    // Numeric Instructions
    case grammar::I32Const::OPCODE:
        return arena_new<I32Const>(arena, inst->as<grammar::I32Const>());
        break;
    case grammar::I64Const::OPCODE:
        return arena_new<I64Const>(arena, inst->as<grammar::I64Const>());
        break;
    case grammar::F32Const::OPCODE:
        return arena_new<F32Const>(arena, inst->as<grammar::F32Const>());
        break;
    case grammar::F64Const::OPCODE:
        return arena_new<F64Const>(arena, inst->as<grammar::F64Const>());
        break;
    case grammar::I32EqualZero::OPCODE:
        return arena_new<I32EqualZero>(arena);
        break;
    case grammar::I32Equal::OPCODE:
        return arena_new<I32Equal>(arena);
        break;
    case grammar::I32NotEqual::OPCODE:
        return arena_new<I32NotEqual>(arena);
        break;
    case grammar::I32LessThanSigned::OPCODE:
        return arena_new<I32LessThanSigned>(arena);
        break;
    case grammar::I32LessThanUnsigned::OPCODE:
        return arena_new<I32LessThanUnsigned>(arena);
        break;
    case grammar::I32GreaterThanSigned::OPCODE:
        return arena_new<I32GreaterThanSigned>(arena);
        break;
    case grammar::I32GreaterThanUnsigned::OPCODE:
        return arena_new<I32GreaterThanUnsigned>(arena);
        break;
    case grammar::I32LessEqualSigned::OPCODE:
        return arena_new<I32LessEqualSigned>(arena);
        break;
    case grammar::I32LessEqualUnsigned::OPCODE:
        return arena_new<I32LessEqualUnsigned>(arena);
        break;
    case grammar::I32GreaterEqualSigned::OPCODE:
        return arena_new<I32GreaterEqualSigned>(arena);
        break;
    case grammar::I32GreaterEqualUnsigned::OPCODE:
        return arena_new<I32GreaterEqualUnsigned>(arena);
        break;
    case grammar::I64EqualZero::OPCODE:
        return arena_new<I64EqualZero>(arena);
        break;
    case grammar::I64Equal::OPCODE:
        return arena_new<I64Equal>(arena);
        break;
    case grammar::I64NotEqual::OPCODE:
        return arena_new<I64NotEqual>(arena);
        break;
    case grammar::I64LessThanSigned::OPCODE:
        return arena_new<I64LessThanSigned>(arena);
        break;
    case grammar::I64LessThanUnsigned::OPCODE:
        return arena_new<I64LessThanUnsigned>(arena);
        break;
    case grammar::I64GreaterThanSigned::OPCODE:
        return arena_new<I64GreaterThanSigned>(arena);
        break;
    case grammar::I64GreaterThanUnsigned::OPCODE:
        return arena_new<I64GreaterThanUnsigned>(arena);
        break;
    case grammar::I64LessEqualSigned::OPCODE:
        return arena_new<I64LessEqualSigned>(arena);
        break;
    case grammar::I64LessEqualUnsigned::OPCODE:
        return arena_new<I64LessEqualUnsigned>(arena);
        break;
    case grammar::I64GreaterEqualSigned::OPCODE:
        return arena_new<I64GreaterEqualSigned>(arena);
        break;
    case grammar::I64GreaterEqualUnsigned::OPCODE:
        return arena_new<I64GreaterEqualUnsigned>(arena);
        break;
    case grammar::F32Equal::OPCODE:
        return arena_new<F32Equal>(arena);
        break;
    case grammar::F32NotEqual::OPCODE:
        return arena_new<F32NotEqual>(arena);
        break;
    case grammar::F32LessThan::OPCODE:
        return arena_new<F32LessThan>(arena);
        break;
    case grammar::F32GreaterThan::OPCODE:
        return arena_new<F32GreaterThan>(arena);
        break;
    case grammar::F32LessEqual::OPCODE:
        return arena_new<F32LessEqual>(arena);
        break;
    case grammar::F32GreaterEqual::OPCODE:
        return arena_new<F32GreaterEqual>(arena);
        break;
    case grammar::F64Equal::OPCODE:
        return arena_new<F64Equal>(arena);
        break;
    case grammar::F64NotEqual::OPCODE:
        return arena_new<F64NotEqual>(arena);
        break;
    case grammar::F64LessThan::OPCODE:
        return arena_new<F64LessThan>(arena);
        break;
    case grammar::F64GreaterThan::OPCODE:
        return arena_new<F64GreaterThan>(arena);
        break;
    case grammar::F64LessEqual::OPCODE:
        return arena_new<F64LessEqual>(arena);
        break;
    case grammar::F64GreaterEqual::OPCODE:
        return arena_new<F64GreaterEqual>(arena);
        break;
    case grammar::I32CountLeadingZeros::OPCODE:
        return arena_new<I32CountLeadingZeros>(arena);
        break;
    case grammar::I32CountTrailingZeros::OPCODE:
        return arena_new<I32CountTrailingZeros>(arena);
        break;
    case grammar::I32PopCount::OPCODE:
        return arena_new<I32PopCount>(arena);
        break;
    case grammar::I32Add::OPCODE:
        return arena_new<I32Add>(arena, inst->as<grammar::I32Add>());
        break;
    case grammar::I32Sub::OPCODE:
        return arena_new<I32Sub>(arena, inst->as<grammar::I32Sub>());
        break;
    case grammar::I32Mul::OPCODE:
        return arena_new<I32Mul>(arena);
        break;
    case grammar::I32DivideSigned::OPCODE:
        return arena_new<I32DivideSigned>(arena);
        break;
    case grammar::I32DivideUnsigned::OPCODE:
        return arena_new<I32DivideUnsigned>(arena);
        break;
    case grammar::I32RemainderSigned::OPCODE:
        return arena_new<I32RemainderSigned>(arena);
        break;
    case grammar::I32RemainderUnsigned::OPCODE:
        return arena_new<I32RemainderUnsigned>(arena);
        break;
    case grammar::I32And::OPCODE:
        return arena_new<I32And>(arena);
        break;
    case grammar::I32Or::OPCODE:
        return arena_new<I32Or>(arena);
        break;
    case grammar::I32Xor::OPCODE:
        return arena_new<I32Xor>(arena);
        break;
    case grammar::I32ShiftLeft::OPCODE:
        return arena_new<I32ShiftLeft>(arena);
        break;
    case grammar::I32ShiftRightSigned::OPCODE:
        return arena_new<I32ShiftRightSigned>(arena);
        break;
    case grammar::I32ShiftRightUnsigned::OPCODE:
        return arena_new<I32ShiftRightUnsigned>(arena);
        break;
    case grammar::I32RotateLeft::OPCODE:
        return arena_new<I32RotateLeft>(arena);
        break;
    case grammar::I32RotateRight::OPCODE:
        return arena_new<I32RotateRight>(arena);
        break;
    case grammar::I64CountLeadingZeros::OPCODE:
        return arena_new<I64CountLeadingZeros>(arena);
        break;
    case grammar::I64CountTrailingZeros::OPCODE:
        return arena_new<I64CountTrailingZeros>(arena);
        break;
    case grammar::I64PopCount::OPCODE:
        return arena_new<I64PopCount>(arena);
        break;
    case grammar::I64Add::OPCODE:
        return arena_new<I64Add>(arena);
        break;
    case grammar::I64Sub::OPCODE:
        return arena_new<I64Sub>(arena);
        break;
    case grammar::I64Mul::OPCODE:
        return arena_new<I64Mul>(arena);
        break;
    case grammar::I64DivideSigned::OPCODE:
        return arena_new<I64DivideSigned>(arena);
        break;
    case grammar::I64DivideUnsigned::OPCODE:
        return arena_new<I64DivideUnsigned>(arena);
        break;
    case grammar::I64RemainderSigned::OPCODE:
        return arena_new<I64RemainderSigned>(arena);
        break;
    case grammar::I64RemainderUnsigned::OPCODE:
        return arena_new<I64RemainderUnsigned>(arena);
        break;
    case grammar::I64And::OPCODE:
        return arena_new<I64And>(arena);
        break;
    case grammar::I64Or::OPCODE:
        return arena_new<I64Or>(arena);
        break;
    case grammar::I64Xor::OPCODE:
        return arena_new<I64Xor>(arena);
        break;
    case grammar::I64ShiftLeft::OPCODE:
        return arena_new<I64ShiftLeft>(arena);
        break;
    case grammar::I64ShiftRightSigned::OPCODE:
        return arena_new<I64ShiftRightSigned>(arena);
        break;
    case grammar::I64ShiftRightUnsigned::OPCODE:
        return arena_new<I64ShiftRightUnsigned>(arena);
        break;
    case grammar::I64RotateLeft::OPCODE:
        return arena_new<I64RotateLeft>(arena);
        break;
    case grammar::I64RotateRight::OPCODE:
        return arena_new<I64RotateRight>(arena);
        break;
    case grammar::F32Mul::OPCODE:
        return arena_new<F32Mul>(arena);
        break;
    case grammar::F32Div::OPCODE:
        return arena_new<F32Div>(arena);
        break;
    case grammar::F64Neg::OPCODE:
        return arena_new<F64Neg>(arena, inst->as<grammar::F64Neg>());
        break;
    case grammar::F64Add::OPCODE:
        return arena_new<F64Add>(arena, inst->as<grammar::F64Add>());
        break;
    case grammar::F64Sub::OPCODE:
        return arena_new<F64Sub>(arena, inst->as<grammar::F64Sub>());
        break;
    case grammar::F64Mul::OPCODE:
        return arena_new<F64Mul>(arena);
        break;
    case grammar::F64Div::OPCODE:
        return arena_new<F64Div>(arena);
        break;
    case grammar::F64CopySign::OPCODE:
        return arena_new<F64CopySign>(arena);
        break;
    case grammar::I32WrapI64::OPCODE:
        return arena_new<I32WrapI64>(arena);
        break;
    case grammar::I64ExtendI32Signed::OPCODE:
        return arena_new<I64ExtendI32Signed>(arena);
        break;
    case grammar::I64ExtendI32Unsigned::OPCODE:
        return arena_new<I64ExtendI32Unsigned>(arena);
        break;
    case grammar::F32ConvertI32Signed::OPCODE:
        return arena_new<F32ConvertI32Signed>(arena);
        break;
    case grammar::F32ConvertI32Unsigned::OPCODE:
        return arena_new<F32ConvertI32Unsigned>(arena);
        break;
    case grammar::F32DemoteF64::OPCODE:
        return arena_new<F32DemoteF64>(arena);
        break;
    case grammar::F64ConvertI32Signed::OPCODE:
        return arena_new<F64ConvertI32Signed>(arena);
        break;
    case grammar::F64ConvertI32Unsigned::OPCODE:
        return arena_new<F64ConvertI32Unsigned>(arena);
        break;
    case grammar::F64ConvertI64Signed::OPCODE:
        return arena_new<F64ConvertI64Signed>(arena);
        break;
    case grammar::F64ConvertI64Unsigned::OPCODE:
        return arena_new<F64ConvertI64Unsigned>(arena);
        break;
    case grammar::F64PromoteF32::OPCODE:
        return arena_new<F64PromoteF32>(arena);
        break;
    case grammar::I32ReinterpretF32::OPCODE:
        return arena_new<I32ReinterpretF32>(arena);
        break;
    case grammar::I64ReinterpretF64::OPCODE:
        return arena_new<I64ReinterpretF64>(arena);
        break;
    case grammar::F32ReinterpretI32::OPCODE:
        return arena_new<F32ReinterpretI32>(arena);
        break;
    case grammar::F64ReinterpretI64::OPCODE:
        return arena_new<F64ReinterpretI64>(arena);
        break;
    case grammar::I32Extend8Signed::OPCODE:
        return arena_new<I32Extend8Signed>(arena);
        break;
    case grammar::I32Extend16Signed::OPCODE:
        return arena_new<I32Extend16Signed>(arena);
        break;
    case grammar::I64Extend8Signed::OPCODE:
        return arena_new<I64Extend8Signed>(arena);
        break;
    case grammar::I64Extend16Signed::OPCODE:
        return arena_new<I64Extend16Signed>(arena);
        break;
    case grammar::I64Extend32Signed::OPCODE:
        return arena_new<I64Extend32Signed>(arena);
        break;
    // Memory Instructions
    case grammar::I32Load::OPCODE:
        return arena_new<I32Load>(arena, inst->as<grammar::I32Load>());
        break;
    case grammar::I64Load::OPCODE:
        return arena_new<I64Load>(arena, inst->as<grammar::I64Load>());
        break;
    case grammar::F32Load::OPCODE:
        return arena_new<F32Load>(arena, inst->as<grammar::F32Load>());
        break;
    case grammar::F64Load::OPCODE:
        return arena_new<F64Load>(arena, inst->as<grammar::F64Load>());
        break;
    case grammar::I32Load8Signed::OPCODE:
        return arena_new<I32Load8Signed>(arena,
                                         inst->as<grammar::I32Load8Signed>());
        break;
    case grammar::I32Load8Unsigned::OPCODE:
        return arena_new<I32Load8Unsigned>(
            arena, inst->as<grammar::I32Load8Unsigned>());
        break;
    case grammar::I32Load16Signed::OPCODE:
        return arena_new<I32Load16Signed>(arena,
                                          inst->as<grammar::I32Load16Signed>());
        break;
    case grammar::I32Load16Unsigned::OPCODE:
        return arena_new<I32Load16Unsigned>(
            arena, inst->as<grammar::I32Load16Unsigned>());
        break;
    case grammar::I64Load8Signed::OPCODE:
        return arena_new<I64Load8Signed>(arena,
                                         inst->as<grammar::I64Load8Signed>());
        break;
    case grammar::I64Load8Unsigned::OPCODE:
        return arena_new<I64Load8Unsigned>(
            arena, inst->as<grammar::I64Load8Unsigned>());
        break;
    case grammar::I64Load16Signed::OPCODE:
        return arena_new<I64Load16Signed>(arena,
                                          inst->as<grammar::I64Load16Signed>());
        break;
    case grammar::I64Load16Unsigned::OPCODE:
        return arena_new<I64Load16Unsigned>(
            arena, inst->as<grammar::I64Load16Unsigned>());
        break;
    case grammar::I64Load32Signed::OPCODE:
        return arena_new<I64Load32Signed>(arena,
                                          inst->as<grammar::I64Load32Signed>());
        break;
    case grammar::I64Load32Unsigned::OPCODE:
        return arena_new<I64Load32Unsigned>(
            arena, inst->as<grammar::I64Load32Unsigned>());
        break;
    case grammar::I32Store::OPCODE:
        return arena_new<I32Store>(arena, inst->as<grammar::I32Store>());
        break;
    case grammar::I64Store::OPCODE:
        return arena_new<I64Store>(arena, inst->as<grammar::I64Store>());
        break;
    case grammar::F32Store::OPCODE:
        return arena_new<F32Store>(arena, inst->as<grammar::F32Store>());
        break;
    case grammar::F64Store::OPCODE:
        return arena_new<F64Store>(arena, inst->as<grammar::F64Store>());
        break;
    case grammar::I32Store8::OPCODE:
        return arena_new<I32Store8>(arena, inst->as<grammar::I32Store8>());
        break;
    case grammar::I32Store16::OPCODE:
        return arena_new<I32Store16>(arena, inst->as<grammar::I32Store16>());
        break;
    case grammar::I64Store8::OPCODE:
        return arena_new<I64Store8>(arena, inst->as<grammar::I64Store8>());
        break;
    case grammar::I64Store16::OPCODE:
        return arena_new<I64Store16>(arena, inst->as<grammar::I64Store16>());
        break;
    case grammar::I64Store32::OPCODE:
        return arena_new<I64Store32>(arena, inst->as<grammar::I64Store32>());
        break;
    case grammar::MemoryGrow::OPCODE:
        return arena_new<MemoryGrow>(arena, inst->as<grammar::MemoryGrow>());
        break;
    case grammar::ExtendedIntstructionBase::OPCODE: {
        const grammar::ExtendedIntstructionBase& ext_inst =
            inst->as<grammar::ExtendedIntstructionBase>();
        uint32_t ext_opcode = ext_inst.getExtendedOpcode();

        switch (ext_opcode) {
        case grammar::I32TruncateSaturateF64Signed::EXT_OPCODE:
            return arena_new<I32TruncateSaturateF64Signed>(
                arena, inst->as<grammar::I32TruncateSaturateF64Signed>());
        case grammar::I32TruncateSaturateF64Unsigned::EXT_OPCODE:
            return arena_new<I32TruncateSaturateF64Unsigned>(
                arena, inst->as<grammar::I32TruncateSaturateF64Unsigned>());
        case grammar::I64TruncateSaturateF64Signed::EXT_OPCODE:
            return arena_new<I64TruncateSaturateF64Signed>(
                arena, inst->as<grammar::I64TruncateSaturateF64Signed>());
        case grammar::MemoryInit::EXT_OPCODE:
            return arena_new<MemoryInit>(arena,
                                         inst->as<grammar::MemoryInit>());
        case grammar::DataDrop::EXT_OPCODE:
            return arena_new<DataDrop>(arena, inst->as<grammar::DataDrop>());
        case grammar::MemoryCopy::EXT_OPCODE:
            return arena_new<MemoryCopy>(arena);
        case grammar::MemoryFill::EXT_OPCODE:
            return arena_new<MemoryFill>(arena,
                                         inst->as<grammar::MemoryFill>());
        default:
            fmt::println("unknown extended opcode 0xFC 0x{:02X}", ext_opcode);
            exit(1);
        }
        break;
    }
    // Atomic Memory Instructions
    case grammar::AtomicIntstructionBase::OPCODE: {
        const grammar::AtomicIntstructionBase& atomic_inst =
            inst->as<grammar::AtomicIntstructionBase>();
        uint32_t atomic_opcode = atomic_inst.getAtomicOpcode();

        switch (atomic_opcode) {
        case grammar::AtomicNotify::ATOMIC_OPCODE:
            return arena_new<AtomicNotify>(arena,
                                           inst->as<grammar::AtomicNotify>());
        case grammar::AtomicWait32::ATOMIC_OPCODE:
            return arena_new<AtomicWait32>(arena,
                                           inst->as<grammar::AtomicWait32>());
        case grammar::AtomicLoad::ATOMIC_OPCODE:
            return arena_new<AtomicLoad>(arena,
                                         inst->as<grammar::AtomicLoad>());
        case grammar::AtomicLoad8Unsigned::ATOMIC_OPCODE:
            return arena_new<AtomicLoad8Unsigned>(
                arena, inst->as<grammar::AtomicLoad8Unsigned>());
        case grammar::AtomicStore::ATOMIC_OPCODE:
            return arena_new<AtomicStore>(arena,
                                          inst->as<grammar::AtomicStore>());
        case grammar::AtomicStore8::ATOMIC_OPCODE:
            return arena_new<AtomicStore8>(arena,
                                           inst->as<grammar::AtomicStore8>());
        case grammar::AtomicAdd::ATOMIC_OPCODE:
            return arena_new<AtomicAdd>(arena, inst->as<grammar::AtomicAdd>());
        case grammar::AtomicExchange8Unsigned::ATOMIC_OPCODE:
            return arena_new<AtomicExchange8Unsigned>(
                arena, inst->as<grammar::AtomicExchange8Unsigned>());
        case grammar::AtomicCompareExchange::ATOMIC_OPCODE:
            return arena_new<AtomicCompareExchange>(
                arena, inst->as<grammar::AtomicCompareExchange>());
        default:
            fmt::println("unknown atomic opcode 0xFE 0x{:02X}", atomic_opcode);
            exit(1);
        }
    }
    default:
        fmt::println("unknown opcode 0x{:02X}", opcode);
        exit(1);
    }
}

Continuation OperationBase::trap(Instance& instance, std::string msg,
                                 size_t addr) const {
    std::string fmt_loc =
        instance.getGlobalState().getDebugInfo().getFormattedLocation(addr);
    fmt::println("trap: {}: at {}", msg, fmt_loc);
    Context& ctxt = instance.getActiveContext();
    const Continuation* epilogues = ctxt.getEpilogues().data();
    for (size_t i = ctxt.getEpilogues().size() - 1; i > 0; i--) {
        Continuation epilogue = epilogues[i];
        if (epilogue == nullptr)
            break;
        fmt::println("  {}: at {}", i, epilogue->getFormattedAddress(instance));
    }

    std::exit(1);
    return nullptr;
}

std::string OperationBase::getFormattedAddress(Instance& instance) const {
    return instance.getGlobalState().getDebugInfo().getFormattedLocation(addr_);
}

void OperationBase::CollectorBase::add(Operation next) {
    std::unordered_set<Operation> visited;

    for (auto it = next; it; it = it->next_) {
        operations_.push_back(it);

        if (visited.contains(it)) {
            for (auto v : visited) {
                fmt::println("v: {}", v->getName());
            }

            fmt::println("cycle: {} -> {}", it->getName(),
                         it->next_->getName());
            __builtin_trap();
        }
        visited.emplace(it);

        /* while (operations_.size() >= 2) {
            auto& first = operations_[operations_.size() - 2];
            auto& second = operations_[operations_.size() - 1];

            if (!canMerge(first, second))
                break;

            Operation merged = merge(first, second);

            operations_.pop_back();
            operations_.back() = merged;
        } */
    }
}

Operation OperationBase::CollectorBase::do_finish() {
    for (size_t i = 0; i + 1 < operations_.size(); ++i) {
        operations_[i]->next_ = operations_[i + 1];
    }

    Operation start = operations_.empty() ? nullptr : operations_[0];
    operations_.clear();
    return start;
}

/***************************/
/* Parametric Instructions */
/***************************/

Drop::Drop(const grammar::Drop& drop)
    : TaggedOperation<Drop>(drop.getAddress()) {}

Continuation Drop::action(Instance& instance) {
    instance.getActiveContext().drop();
    TRACE_VERBOSE("drop");
    return next_;
}

Continuation Select::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t cond = context.pop().i32;
    Value b = context.pop();
    Value a = context.pop();
    Value result = cond ? a : b;
    context.push(result);

    TRACE_VERBOSE("select: (a={}, b={}, cond={}) -> ({})", a.i64, b.i64, cond,
                  cond ? a.i64 : b.i64);
    return next_;
}

/***********************/
/* Memory Instructions */
/***********************/

I32Load::I32Load(const grammar::I32Load& i32_load)
    : TaggedOperation<I32Load>(i32_load.getAddress()),
      offset_(i32_load.getMemArg().getOffset()),
      align_(i32_load.getMemArg().getAlign()) {}

Continuation I32Load::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    int32_t value;
    if (offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();

        if (!memory.load(offset, value))
            return trap(instance, "i32.load out of bounds memory address",
                        addr_);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        if (!mmu.load(offset, value)) {
            // repeat the operation after handling the page fault
            context.push(static_cast<int32_t>(base));
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, offset, false);
        }
    }

    context.pushI32(value);

    TRACE_VERBOSE("i32.load {} {}: ({}) -> ({})", align_, offset_, base, value);
    return next_;
}

I64Load::I64Load(const grammar::I64Load& i64_load)
    : offset_(i64_load.getMemArg().getOffset()),
      align_(i64_load.getMemArg().getAlign()) {}

Continuation I64Load::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    int64_t value;
    if (offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.load(offset, value))
            return trap(instance, "i64.load out of bounds memory address",
                        offset);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        if (!mmu.load(offset, value)) {
            // repeat the operation after handling the page fault
            context.push(static_cast<int32_t>(base));
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, offset, false);
        }
    }

    context.pushI64(value);

    TRACE_VERBOSE("i64.load {} {}: ({}) -> ({})", align_, offset_, base, value);
    return next_;
}

F32Load::F32Load(const grammar::F32Load& f32_load)
    : offset_(f32_load.getMemArg().getOffset()),
      align_(f32_load.getMemArg().getAlign()) {}

Continuation F32Load::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    float value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "f32.load out of bounds memory address", offset);

    context.push(value);

    TRACE_VERBOSE("f32.load {} {}: ({}) -> ({})", align_, offset_, base, value);
    return next_;
}

F64Load::F64Load(const grammar::F64Load& f64_load)
    : offset_(f64_load.getMemArg().getOffset()),
      align_(f64_load.getMemArg().getAlign()) {}

Continuation F64Load::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    double val;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, val))
        return trap(instance, "f64.load out of bounds memory address", offset);

    context.push(val);

    TRACE_VERBOSE("f64.load {} {}: ({}) -> ({})", align_, offset_, base, val);
    return next_;
}

I32Load8Signed::I32Load8Signed(const grammar::I32Load8Signed& i32_load8_s)
    : offset_(i32_load8_s.getMemArg().getOffset()),
      align_(i32_load8_s.getMemArg().getAlign()) {}

Continuation I32Load8Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    int8_t value;
    if (offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();

        if (!memory.load(offset, value))
            return trap(instance, "i32.load8_s out of bounds memory address",
                        addr_);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        if (!mmu.load(offset, value)) {
            return trap(instance, "i32.load8_s invalid memory access", addr_);
        }
    }

    context.pushI32(value);
    TRACE_VERBOSE("i32.load8_s {} {}: ({}) -> ({})", align_, offset_, base,
                  value);
    return next_;
}

I32Load8Unsigned::I32Load8Unsigned(const grammar::I32Load8Unsigned& i32_load8_u)
    : offset_(i32_load8_u.getMemArg().getOffset()),
      align_(i32_load8_u.getMemArg().getAlign()) {}

Continuation I32Load8Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    uint8_t value;
    if (offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.load(offset, value))
            return trap(instance, "i32.load8_u out of bounds memory address",
                        addr_);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        if (!mmu.load(offset, value)) {
            // repeat the operation after handling the page fault
            context.pushI32(base);
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, offset, false);
        }
    }

    context.pushI32(value);
    TRACE_VERBOSE("i32.load8_u {} {}: ({}) -> ({})", align_, offset_, base,
                  value);
    return next_;
}

I32Load16Signed::I32Load16Signed(const grammar::I32Load16Signed& i32_load16_s)
    : offset_(i32_load16_s.getMemArg().getOffset()),
      align_(i32_load16_s.getMemArg().getAlign()) {}

Continuation I32Load16Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    int16_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i32.load16_s out of bounds memory address",
                    offset);

    context.pushI32(value);

    TRACE_VERBOSE("i32.load16_s align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_;
}

I32Load16Unsigned::I32Load16Unsigned(
    const grammar::I32Load16Unsigned& i32_load16_u)
    : offset_(i32_load16_u.getMemArg().getOffset()),
      align_(i32_load16_u.getMemArg().getAlign()) {}

Continuation I32Load16Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    uint16_t value;

    if (offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.load(offset, value))
            return trap(instance, "i32.load16_u out of bounds memory address",
                        offset);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        if (!mmu.load(offset, value)) {
            // repeat the operation after handling the page fault
            context.push(static_cast<int32_t>(base));
            context.getEpilogues().push(this);

            // trigger a page fault
            return mmu.fault(instance, offset, false);
        }
    }

    context.pushI32(value);

    TRACE_VERBOSE("i32.load16_u align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_;
}

I64Load8Signed::I64Load8Signed(const grammar::I64Load8Signed& i64_load8_s)
    : offset_(i64_load8_s.getOffset()), align_(i64_load8_s.getAlign()) {}

Continuation I64Load8Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    int8_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load8_s out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load8_s align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_;
}

I64Load8Unsigned::I64Load8Unsigned(const grammar::I64Load8Unsigned& i64_load8_u)
    : offset_(i64_load8_u.getOffset()), align_(i64_load8_u.getAlign()) {}

Continuation I64Load8Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    uint8_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load8_u out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load8_u align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_;
}

I64Load16Signed::I64Load16Signed(const grammar::I64Load16Signed& i64_load16_s)
    : offset_(i64_load16_s.getOffset()), align_(i64_load16_s.getAlign()) {}

Continuation I64Load16Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    int16_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load16_s out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load16_s align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_;
}

I64Load16Unsigned::I64Load16Unsigned(
    const grammar::I64Load16Unsigned& i64_load16_u)
    : offset_(i64_load16_u.getOffset()), align_(i64_load16_u.getAlign()) {}

Continuation I64Load16Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    uint16_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load16_u out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load16_u align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_;
}

I64Load32Signed::I64Load32Signed(const grammar::I64Load32Signed& i64_load32_s)
    : offset_(i64_load32_s.getOffset()), align_(i64_load32_s.getAlign()) {}

Continuation I64Load32Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    int32_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load32_s out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load32_s align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_;
}

I64Load32Unsigned::I64Load32Unsigned(
    const grammar::I64Load32Unsigned& i64_load32_u)
    : offset_(i64_load32_u.getOffset()), align_(i64_load32_u.getAlign()) {}

Continuation I64Load32Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);
    uint32_t offset = base + offset_;

    uint32_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load32_u out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load32_u align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_;
}

I32Store::I32Store(const grammar::I32Store& i32_store)
    : TaggedOperation<I32Store>(i32_store.getAddress()),
      offset_(i32_store.getMemArg().getOffset()),
      align_(i32_store.getMemArg().getAlign()) {}

Continuation I32Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t value = context.pop().i32;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;

    if (offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.store(offset, value))
            return trap(instance, "i32.store out of bounds memory address",
                        addr_);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        if (!mmu.store(offset, value)) {
            // repeat the operation after handling the page fault
            context.pushI32(static_cast<int32_t>(base));
            context.pushI32(value);
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, offset, true);
        }
    }

    TRACE_VERBOSE("i32.store {} {}: ({}, {}) -> ()", align_, offset_, base,
                  value);
    return next_;
}

I64Store::I64Store(const grammar::I64Store& i64_store)
    : offset_(i64_store.getMemArg().getOffset()),
      align_(i64_store.getMemArg().getAlign()) {}

Continuation I64Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t value = context.pop().i64;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;

    if (offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.store(offset, value))
            return trap(instance, "i64.store out of bounds memory address",
                        offset);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        if (!mmu.store(offset, value)) {
            // repeat the operation after handling the page fault
            context.pushI32(static_cast<int32_t>(base));
            context.pushI64(value);
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, offset, true);
        }
    }

    TRACE_VERBOSE("i64.store {} {}: ({}, {}) -> ()", align_, offset_, base,
                  value);
    return next_;
}

F32Store::F32Store(const grammar::F32Store& f32_store)
    : offset_(f32_store.getOffset()), align_(f32_store.getAlign()) {}

Continuation F32Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float value = context.pop().f32;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.store(offset, value))
        return trap(instance, "f32.store out of bounds memory address", offset);

    TRACE_VERBOSE("f32.store {} {}: ({}, {}) -> ()", align_, offset_, base,
                  value);
    return next_;
}

F64Store::F64Store(const grammar::F64Store& f64_store)
    : offset_(f64_store.getOffset()), align_(f64_store.getAlign()) {}

Continuation F64Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double value = context.pop().f64;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.store(offset, value))
        return trap(instance, "f64.store out of bounds memory address", offset);

    TRACE_VERBOSE("f64.store {} {}: ({}, {}) -> ()", align_, offset_, base,
                  value);
    return next_;
}

I32Store8::I32Store8(const grammar::I32Store8& i32_store8)
    : offset_(i32_store8.getMemArg().getOffset()),
      align_(i32_store8.getMemArg().getAlign()) {}

Continuation I32Store8::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t value = context.pop().i32;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;

    if (offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.store(offset, static_cast<uint8_t>(value & 0xFF)))
            return trap(instance, "i32.store8 out of bounds memory address",
                        offset);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        if (!mmu.store(offset, static_cast<uint8_t>(value & 0xFF))) {
            // repeat the operation after handling the page fault
            context.pushI32(static_cast<int32_t>(base));
            context.pushI32(value);
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, offset, true);
        }
    }

    TRACE_VERBOSE("i32.store8 {} {}: ({}, {}) -> ()", align_, offset_, base,
                  value);
    return next_;
}

I32Store16::I32Store16(const grammar::I32Store16& i32_store16)
    : offset_(i32_store16.getMemArg().getOffset()),
      align_(i32_store16.getMemArg().getAlign()) {}

Continuation I32Store16::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t value = context.pop().i32;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;

    if (offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.store(offset, static_cast<uint16_t>(value & 0xFFFF)))
            return trap(instance, "i32.store16 out of bounds memory address",
                        offset);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        if (!mmu.store(offset, static_cast<int16_t>(value & 0xFFFF))) {
            // repeat the operation after handling the page fault
            context.pushI32(static_cast<int32_t>(base));
            context.pushI32(value);
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, offset, true);
        }
    }

    TRACE_VERBOSE("i32.store16 align={} offset={}: (base={}, val={}) -> ()",
                  align_, offset_, base, value & 0xFFFF);
    return next_;
}

I64Store8::I64Store8(const grammar::I64Store8& i64_store8)
    : offset_(i64_store8.getMemArg().getOffset()),
      align_(i64_store8.getMemArg().getAlign()) {}

Continuation I64Store8::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t val = context.pop().i64;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.store(offset, static_cast<uint8_t>(val & 0xFF)))
        return trap(instance, "i64.store8 out of bounds memory address",
                    offset);

    TRACE_VERBOSE("i64.store8 align={} offset={}: (base={}, val={}) -> ()",
                  align_, offset_, base, val & 0xFF);
    return next_;
}

I64Store16::I64Store16(const grammar::I64Store16& i64_store16)
    : offset_(i64_store16.getMemArg().getOffset()),
      align_(i64_store16.getMemArg().getAlign()) {}

Continuation I64Store16::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t value = context.pop().i64;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.store(offset, static_cast<uint16_t>(value & 0xFFFF)))
        return trap(instance, "i64.store16 out of bounds memory address",
                    offset);

    TRACE_VERBOSE("i64.store16 align={} offset={}: (base={}, val={}) -> ()",
                  align_, offset_, base, value & 0xFFFF);
    return next_;
}

I64Store32::I64Store32(const grammar::I64Store32& i64_store32)
    : offset_(i64_store32.getMemArg().getOffset()),
      align_(i64_store32.getMemArg().getAlign()) {}

Continuation I64Store32::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t value = context.pop().i64;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;

    if (offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.store(offset, static_cast<uint32_t>(value & 0xFFFFFFFF)))
            return trap(instance, "i64.store32 out of bounds memory address",
                        offset);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        if (!mmu.store(offset, static_cast<uint32_t>(value & 0xFFFFFFFF))) {
            // repeat the operation after handling the page fault
            context.pushI32(static_cast<int32_t>(base));
            context.pushI64(value);
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, offset, true);
        }
    }

    return next_;
}

MemoryGrow::MemoryGrow(const grammar::MemoryGrow& mem_grow)
    : OperationBase(mem_grow.getAddress()) {}

Continuation MemoryGrow::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t delta = static_cast<uint32_t>(context.pop().i32);
    Memory& memory = instance.getGlobalState().getMemory();

    context.pushI32(static_cast<int32_t>(memory.grow(delta)));
    return next_;
}

MemoryInit::MemoryInit(const grammar::MemoryInit& mem_init)
    : segment_idx_(mem_init.getSegmentIdx()) {}

Continuation MemoryInit::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t len = static_cast<uint32_t>(context.pop().i32);
    uint32_t src_offset = static_cast<uint32_t>(context.pop().i32);
    int32_t dst_offset = context.pop().i32;

    std::vector<uint8_t>& data_segment =
        instance.getGlobalState().getData()[segment_idx_];

    assert(src_offset + len <= data_segment.size());
    auto src_buffer = std::span<uint8_t>(data_segment).subspan(src_offset, len);

    Memory& memory = instance.getGlobalState().getMemory();
    bool result = memory.store(static_cast<uint32_t>(dst_offset), src_buffer);
    assert(result);

    return next_;
}

DataDrop::DataDrop(const grammar::DataDrop& data_drop)
    : segment_idx_(data_drop.getSegmentIdx()) {}

Continuation DataDrop::action(Instance& instance) {
    instance.getGlobalState().dropSegment(segment_idx_);
    return next_;
}

Continuation MemoryCopy::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t count = static_cast<uint32_t>(context.pop().i32);
    uint32_t src_offset = static_cast<uint32_t>(context.pop().i32);
    uint32_t dst_offset = static_cast<uint32_t>(context.pop().i32);

    uint64_t src_end = uint64_t{src_offset} + count;
    uint64_t dst_end = uint64_t{dst_offset} + count;

    if (src_end > UINT32_MAX || dst_end > UINT32_MAX) {
        return trap(instance, "memory.copy out of bounds memory address",
                    addr_);
    }

    std::vector<uint8_t> buffer(count);

    if (src_offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.load(src_offset, buffer))
            return trap(instance, "memory.copy out of bounds memory address",
                        addr_);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();

        MemoryManagementUnit& mmu = kernel.getMMU();
        uint32_t faulting_addr;

        if (!mmu.load(src_offset, buffer, faulting_addr)) {
            // repeat the operation after handling the page fault
            context.pushI32(static_cast<int32_t>(dst_offset));
            context.pushI32(static_cast<int32_t>(src_offset));
            context.pushI32(static_cast<int32_t>(count));
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, faulting_addr, false);
        }
    }

    if (dst_offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.store(dst_offset, buffer))
            return trap(instance, "memory.copy out of bounds memory address",
                        addr_);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();

        MemoryManagementUnit& mmu = kernel.getMMU();
        uint32_t faulting_addr;

        if (!mmu.store(dst_offset, buffer, faulting_addr)) {
            // repeat the operation after handling the page fault
            context.pushI32(static_cast<int32_t>(dst_offset));
            context.pushI32(static_cast<int32_t>(src_offset));
            context.pushI32(static_cast<int32_t>(count));
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, faulting_addr, true);
        }
    }

    TRACE_VERBOSE("memory.copy: (dst={}, src={}, count={})", dst_offset,
                  src_offset, count);
    return next_;
}

Continuation MemoryFill::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t count = static_cast<uint32_t>(context.pop().i32);
    int32_t value = context.pop().i32;
    uint32_t dst_offset = static_cast<uint32_t>(context.pop().i32);

    if (dst_offset < VIRT_MEMORY && dst_offset + count > VIRT_MEMORY) {
        return trap(instance,
                    fmt::format("memory.fill crosses into invalid memory "
                                "region: dst=0x{:x} count={}",
                                dst_offset, count),
                    addr_);
    }

    if (dst_offset < VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.fill(dst_offset, static_cast<uint8_t>(value), count))
            return trap(instance,
                        fmt::format("memory.fill out of bounds memory address: "
                                    "dst=0x{:x} count={}",
                                    dst_offset, count),
                        addr_);
    } else {
        Kernel& kernel = instance.is<Kernel>()
                             ? instance.as<Kernel>()
                             : instance.as<Process>().getKernel();
        MemoryManagementUnit& mmu = kernel.getMMU();
        uint32_t faulting_addr;
        if (!mmu.fill(dst_offset, static_cast<uint8_t>(value), count,
                      faulting_addr)) {
            // repeat the operation after handling the page fault
            context.pushI32(static_cast<int32_t>(dst_offset));
            context.pushI32(value);
            context.pushI32(static_cast<int32_t>(count));
            context.getEpilogues().push(this);

            // trigger page fault handling
            return mmu.fault(instance, faulting_addr, true);
        }
    }

    return next_;
};

/******************************/
/* Atomic Memory Instructions */
/******************************/

AtomicNotify::AtomicNotify(const grammar::AtomicNotify& atomic_notify)
    : offset_(atomic_notify.getMemArg().getOffset()),
      align_(atomic_notify.getMemArg().getAlign()) {}

Continuation AtomicNotify::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t waiters = static_cast<uint32_t>(context.pop().i32);
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;
    if (offset % align_ != 0)
        return trap(instance, "memory.atomic.notify misaligned memory address",
                    addr_);

    Memory& memory = instance.getGlobalState().getMemory();
    if (memory.contains(offset, sizeof(int32_t)))
        return trap(instance,
                    "memory.atomic.notify out of bounds memory address", addr_);

    WaitingRoom& waiting_room = instance.getWaitingRoom();
    uint32_t notified = waiting_room.notify(offset, waiters);

    context.pushI32(static_cast<int32_t>(notified));
    return next_;
}

AtomicWait32::AtomicWait32(const grammar::AtomicWait32& atomic_wait32)
    : offset_(atomic_wait32.getMemArg().getOffset()),
      align_(atomic_wait32.getMemArg().getAlign()) {}

Continuation AtomicWait32::action(Instance& instance) {
    Context& context = instance.getActiveContext();
    WaitingRoom& waiting_room = instance.getWaitingRoom();

    if (context.getRunState() == Context::RunState::waiting) {
        if (!waiting_room.isBlocked(context, context.getWaitAddr())) {
            context.setRunState(Context::RunState::running);
            context.pushI32(0);

            TRACE_VERBOSE("memory.atomic.wait32 epilogue {} {}: () -> ({})",
                          align_, offset_, 0);
            return next_;
        }

        if (context.getRemainingTime() == 0) {
            context.setRunState(Context::RunState::running);
            context.pushI32(2);

            TRACE_VERBOSE("memory.atomic.wait32 epilogue {} {}: () -> ({})",
                          align_, offset_, 2);
            return next_;
        }

        return idle();
    }

    int64_t timeout = context.pop().i64;
    int32_t expected = context.pop().i32;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;
    if (offset % align_ != 0)
        return trap(instance, "memory.atomic.wait32 misaligned memory address",
                    addr_);

    int32_t* value_ptr;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.ptr(offset, &value_ptr))
        return trap(instance,
                    "memory.atomic.wait32 out of bounds memory address", addr_);

    int32_t value = __atomic_load_n(value_ptr, __ATOMIC_SEQ_CST);
    TRACE_VERBOSE("memory.atomic.wait32 prologue {} {}: ({}, {}, {}) -> ()",
                  align_, offset_, timeout, expected, base);

    if (value != expected) {
        context.pushI32(1);

        TRACE_VERBOSE("memory.atomic.wait32 epilogue {} {}: () -> ({})", align_,
                      offset_, 1);
        return next_;
    } else {
        waiting_room.block(context, offset);
        context.setTimeout(offset, timeout);
        context.setRunState(Context::RunState::waiting);

        // TODO: figure out what i wanted to do here?
        // idle_->addNext(shared_from_this());
        __builtin_trap();
        return idle();
    }
}

Continuation AtomicWait32::idle() const {
    static Nop nop(UINT32_MAX);
    return &nop;
}

AtomicLoad::AtomicLoad(const grammar::AtomicLoad& atomic_load)
    : offset_(atomic_load.getMemArg().getOffset()),
      align_(atomic_load.getMemArg().getAlign()) {}

Continuation AtomicLoad::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t addr = base + offset_;
    if (addr % align_ != 0)
        return trap(instance, "memory.atomic.load misaligned memory address",
                    addr_);

    int32_t* value_ptr;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.ptr(addr, &value_ptr))
        return trap(instance, "memory.atomic.load out of bounds memory address",
                    addr_);

    int32_t value = __atomic_load_n(value_ptr, __ATOMIC_SEQ_CST);

    context.pushI32(value);

    TRACE_VERBOSE("memory.atomic.load {} {}: ({}) -> ({})", align_, offset_,
                  base, value);
    return next_;
}

AtomicLoad8Unsigned::AtomicLoad8Unsigned(
    const grammar::AtomicLoad8Unsigned& atomic_load8_u)
    : offset_(atomic_load8_u.getMemArg().getOffset()),
      align_(atomic_load8_u.getMemArg().getAlign()) {}

Continuation AtomicLoad8Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t addr = base + offset_;
    if (addr % align_ != 0)
        return trap(instance, "memory.atomic.load8_u misaligned memory address",
                    addr_);

    uint8_t* value_ptr;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.ptr(addr, &value_ptr))
        return trap(instance, "memory.atomic.load8_u misaligned memory address",
                    addr_);

    uint8_t value = __atomic_load_n(value_ptr, __ATOMIC_SEQ_CST);

    context.pushI32(value);

    TRACE_VERBOSE("memory.atomic.load8_u {} {}: ({}) -> ({})", align_, offset_,
                  base, value);
    return next_;
}

AtomicStore::AtomicStore(const grammar::AtomicStore& atomic_store)
    : offset_(atomic_store.getMemArg().getOffset()),
      align_(atomic_store.getMemArg().getAlign()) {}

Continuation AtomicStore::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t value = context.pop().i32;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;
    if (offset % align_ != 0)
        return trap(instance, "memory.atomic.store misaligned memory address",
                    addr_);

    int32_t* value_ptr;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.ptr(offset, &value_ptr))
        return trap(instance,
                    "memory.atomic.store out of bounds memory address", addr_);

    __atomic_store_n(value_ptr, value, __ATOMIC_SEQ_CST);

    TRACE_VERBOSE("memory.atomic.store {} {}: ({}, {}) -> ()", align_, offset_,
                  value, base);
    return next_;
}

AtomicStore8::AtomicStore8(const grammar::AtomicStore8& atomic_store8)
    : offset_(atomic_store8.getMemArg().getOffset()),
      align_(atomic_store8.getMemArg().getAlign()) {}

Continuation AtomicStore8::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint8_t value = static_cast<uint8_t>(context.pop().i32);
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;
    if (offset % align_ != 0)
        return trap(instance, "memory.atomic.store8 misaligned memory address",
                    addr_);

    uint8_t* value_ptr;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.ptr(offset, &value_ptr))
        return trap(instance,
                    "memory.atomic.store8 out of bounds memory address", addr_);
    ;
    __atomic_store_n(value_ptr, value, __ATOMIC_SEQ_CST);

    TRACE_VERBOSE("memory.atomic.store8 {} {}: ({}, {}) -> ()", align_, offset_,
                  value, base);
    return next_;
}

AtomicAdd::AtomicAdd(const grammar::AtomicAdd& atomic_add)
    : offset_(atomic_add.getMemArg().getOffset()),
      align_(atomic_add.getMemArg().getAlign()) {}

Continuation AtomicAdd::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t value = context.pop().i32;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;
    if (offset % align_ != 0)
        return trap(instance, "i32.atomic.rmw.add misaligned memory address",
                    addr_);

    uint32_t* value_ptr;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.ptr(offset, &value_ptr))
        return trap(instance, "i32.atomic.rmw.add out of bounds memory address",
                    addr_);

    uint32_t old = __atomic_fetch_add(value_ptr, static_cast<uint32_t>(value),
                                      __ATOMIC_SEQ_CST);

    context.pushI32(static_cast<int32_t>(old));

    TRACE_VERBOSE("i32.atomic.rmw.add {} {}: ({}, {}) -> ()", align_, offset_,
                  base, value, old);
    return next_;
}

AtomicExchange8Unsigned::AtomicExchange8Unsigned(
    const grammar::AtomicExchange8Unsigned& atomic_xchg8_u)
    : offset_(atomic_xchg8_u.getMemArg().getOffset()),
      align_(atomic_xchg8_u.getMemArg().getAlign()) {}

Continuation AtomicExchange8Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint8_t value = static_cast<uint8_t>(context.pop().i32);
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;
    if (offset % align_ != 0)
        return trap(instance,
                    "i32.atomic.rmw8.xchg_u misaligned memory address", addr_);

    uint8_t* value_ptr;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.ptr(offset, &value_ptr))
        return trap(instance,
                    "i32.atomic.rmw8.xchg_u out of bonds memory address",
                    addr_);

    uint8_t old = __atomic_exchange_n(value_ptr, value, __ATOMIC_SEQ_CST);
    context.pushI32(old);

    TRACE_VERBOSE("i32.atomic.rmw8.xchg_u {} {}: ({}, {}) -> ({})", align_,
                  offset_, value, base, old);
    return next_;
}

AtomicCompareExchange::AtomicCompareExchange(
    const grammar::AtomicCompareExchange& atomic_cmpxchg)
    : offset_(atomic_cmpxchg.getMemArg().getOffset()),
      align_(atomic_cmpxchg.getMemArg().getAlign()) {}

Continuation AtomicCompareExchange::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t desired = context.pop().i32;
    int32_t expected = context.pop().i32;
    uint32_t base = static_cast<uint32_t>(context.pop().i32);

    uint32_t offset = base + offset_;
    if (offset % align_ != 0)
        return trap(instance,
                    "i32.atomic.rmw.cmpxchg misaligned memory address", addr_);

    int32_t* ptr;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.ptr(offset, &ptr))
        return trap(instance,
                    "i32.atomic.rmw.cmpxchg out of bounds memory address",
                    addr_);

    __atomic_compare_exchange_n(ptr, &expected, desired, false,
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

    context.pushI32(expected);
    return next_;
}

} // namespace runtime
