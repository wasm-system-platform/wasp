#include <fmt/base.h>
#include <list>

#include "hw/mem/mmu.hpp"
#include "runtime/context.hpp"
#include "runtime/instance.hpp"
#include "runtime/kernel.hpp"
#include "runtime/operations.hpp"
#include "runtime/operations_impl.hpp"
#include "runtime/optimization.hpp"
#include "runtime/vm.hpp"

namespace runtime {

using hw::mem::VIRT_MEMORY;

Operation
OperationBase::create(const std::vector<grammar::Instruction>& instructions,
                      const std::vector<FunctionType>& func_types,
                      std::vector<Operation>& targets) {
    Builder builder;

    for (size_t i = 0; i < instructions.size(); ++i) {
        Operation next;

        const grammar::Instruction& inst = instructions[i];
        uint8_t opcode = inst->getOpcode();

        switch (opcode) {
        // Control Instructions
        case grammar::Unreachable::OPCODE:
            next =
                std::make_shared<Unreachable>(inst->as<grammar::Unreachable>());
            break;
        case grammar::Nop::OPCODE:
            next = std::make_shared<Nop>(inst->as<grammar::Nop>());
            break;
        case grammar::Block::OPCODE:
            next = std::make_shared<Block>(inst->as<grammar::Block>(),
                                           func_types, targets);
            break;
        case grammar::Loop::OPCODE:
            next = std::make_shared<Loop>(inst->as<grammar::Loop>(), func_types,
                                          targets);
            break;
        case grammar::IfElse::OPCODE: {
            grammar::IfElse if_else_inst = inst->as<grammar::IfElse>();

            if (if_else_inst.hasElseExpression())
                next =
                    std::make_shared<IfElse>(if_else_inst, func_types, targets);
            else
                next =
                    std::make_shared<IfThen>(if_else_inst, func_types, targets);

            break;
        }
        case grammar::Branch::OPCODE:
            next =
                std::make_shared<Branch>(inst->as<grammar::Branch>(), targets);
            break;
        case grammar::BranchIf::OPCODE:
            next = std::make_shared<BranchIf>(inst->as<grammar::BranchIf>(),
                                              targets);
            break;
        case grammar::BranchTable::OPCODE:
            next = std::make_shared<BranchTable>(
                inst->as<grammar::BranchTable>(), targets);
            break;
        case grammar::Call::OPCODE:
            next = std::make_shared<Call>(inst->as<grammar::Call>());
            break;
        case grammar::CallIndirect::OPCODE:
            next = std::make_shared<CallIndirect>(
                inst->as<grammar::CallIndirect>(), func_types);
            break;
        case grammar::Return::OPCODE:
            next = std::make_shared<Return>();
            break;
        // Parametric Instructions
        case grammar::Drop::OPCODE:
            next = std::make_shared<Drop>(inst->as<grammar::Drop>());
            break;
        case grammar::Select::OPCODE:
            next = std::make_shared<Select>();
            break;
        // Variable Instruction
        case grammar::LocalGet::OPCODE:
            next = std::make_shared<LocalGet>(inst->as<grammar::LocalGet>());
            break;
        case grammar::LocalSet::OPCODE:
            next = std::make_shared<LocalSet>(inst->as<grammar::LocalSet>());
            break;
        case grammar::LocalTee::OPCODE:
            next = std::make_shared<LocalTee>(inst->as<grammar::LocalTee>());
            break;
        case grammar::GlobalGet::OPCODE:
            next = std::make_shared<GlobalGet>(inst->as<grammar::GlobalGet>());
            break;
        case grammar::GlobalSet::OPCODE:
            next = std::make_shared<GlobalSet>(inst->as<grammar::GlobalSet>());
            break;
        // Numeric Instructions
        case grammar::I32Const::OPCODE:
            next = std::make_shared<I32Const>(inst->as<grammar::I32Const>());
            break;
        case grammar::I64Const::OPCODE:
            next = std::make_shared<I64Const>(inst->as<grammar::I64Const>());
            break;
        case grammar::F32Const::OPCODE:
            next = std::make_shared<F32Const>(inst->as<grammar::F32Const>());
            break;
        case grammar::F64Const::OPCODE:
            next = std::make_shared<F64Const>(inst->as<grammar::F64Const>());
            break;
        case grammar::I32EqualZero::OPCODE:
            next = std::make_shared<I32EqualZero>();
            break;
        case grammar::I32Equal::OPCODE:
            next = std::make_shared<I32Equal>();
            break;
        case grammar::I32NotEqual::OPCODE:
            next = std::make_shared<I32NotEqual>();
            break;
        case grammar::I32LessThanSigned::OPCODE:
            next = std::make_shared<I32LessThanSigned>();
            break;
        case grammar::I32LessThanUnsigned::OPCODE:
            next = std::make_shared<I32LessThanUnsigned>();
            break;
        case grammar::I32GreaterThanSigned::OPCODE:
            next = std::make_shared<I32GreaterThanSigned>();
            break;
        case grammar::I32GreaterThanUnsigned::OPCODE:
            next = std::make_shared<I32GreaterThanUnsigned>();
            break;
        case grammar::I32LessEqualSigned::OPCODE:
            next = std::make_shared<I32LessEqualSigned>();
            break;
        case grammar::I32LessEqualUnsigned::OPCODE:
            next = std::make_shared<I32LessEqualUnsigned>();
            break;
        case grammar::I32GreaterEqualSigned::OPCODE:
            next = std::make_shared<I32GreaterEqualSigned>();
            break;
        case grammar::I32GreaterEqualUnsigned::OPCODE:
            next = std::make_shared<I32GreaterEqualUnsigned>();
            break;
        case grammar::I64EqualZero::OPCODE:
            next = std::make_shared<I64EqualZero>();
            break;
        case grammar::I64Equal::OPCODE:
            next = std::make_shared<I64Equal>();
            break;
        case grammar::I64NotEqual::OPCODE:
            next = std::make_shared<I64NotEqual>();
            break;
        case grammar::I64LessThanSigned::OPCODE:
            next = std::make_shared<I64LessThanSigned>();
            break;
        case grammar::I64LessThanUnsigned::OPCODE:
            next = std::make_shared<I64LessThanUnsigned>();
            break;
        case grammar::I64GreaterThanSigned::OPCODE:
            next = std::make_shared<I64GreaterThanSigned>();
            break;
        case grammar::I64GreaterThanUnsigned::OPCODE:
            next = std::make_shared<I64GreaterThanUnsigned>();
            break;
        case grammar::I64LessEqualSigned::OPCODE:
            next = std::make_shared<I64LessEqualSigned>();
            break;
        case grammar::I64LessEqualUnsigned::OPCODE:
            next = std::make_shared<I64LessEqualUnsigned>();
            break;
        case grammar::I64GreaterEqualSigned::OPCODE:
            next = std::make_shared<I64GreaterEqualSigned>();
            break;
        case grammar::I64GreaterEqualUnsigned::OPCODE:
            next = std::make_shared<I64GreaterEqualUnsigned>();
            break;
        case grammar::F32Equal::OPCODE:
            next = std::make_shared<F32Equal>();
            break;
        case grammar::F32NotEqual::OPCODE:
            next = std::make_shared<F32NotEqual>();
            break;
        case grammar::F32LessThan::OPCODE:
            next = std::make_shared<F32LessThan>();
            break;
        case grammar::F32GreaterThan::OPCODE:
            next = std::make_shared<F32GreaterThan>();
            break;
        case grammar::F32LessEqual::OPCODE:
            next = std::make_shared<F32LessEqual>();
            break;
        case grammar::F32GreaterEqual::OPCODE:
            next = std::make_shared<F32GreaterEqual>();
            break;
        case grammar::F64Equal::OPCODE:
            next = std::make_shared<F64Equal>();
            break;
        case grammar::F64NotEqual::OPCODE:
            next = std::make_shared<F64NotEqual>();
            break;
        case grammar::F64LessThan::OPCODE:
            next = std::make_shared<F64LessThan>();
            break;
        case grammar::F64GreaterThan::OPCODE:
            next = std::make_shared<F64GreaterThan>();
            break;
        case grammar::F64LessEqual::OPCODE:
            next = std::make_shared<F64LessEqual>();
            break;
        case grammar::F64GreaterEqual::OPCODE:
            next = std::make_shared<F64GreaterEqual>();
            break;
        case grammar::I32CountLeadingZeros::OPCODE:
            next = std::make_shared<I32CountLeadingZeros>();
            break;
        case grammar::I32CountTrailingZeros::OPCODE:
            next = std::make_shared<I32CountTrailingZeros>();
            break;
        case grammar::I32PopCount::OPCODE:
            next = std::make_shared<I32PopCount>();
            break;
        case grammar::I32Add::OPCODE:
            next = std::make_shared<I32Add>(inst->as<grammar::I32Add>());
            break;
        case grammar::I32Sub::OPCODE:
            next = std::make_shared<I32Sub>(inst->as<grammar::I32Sub>());
            break;
        case grammar::I32Mul::OPCODE:
            next = std::make_shared<I32Mul>();
            break;
        case grammar::I32DivideSigned::OPCODE:
            next = std::make_shared<I32DivideSigned>();
            break;
        case grammar::I32DivideUnsigned::OPCODE:
            next = std::make_shared<I32DivideUnsigned>();
            break;
        case grammar::I32RemainderSigned::OPCODE:
            next = std::make_shared<I32RemainderSigned>();
            break;
        case grammar::I32RemainderUnsigned::OPCODE:
            next = std::make_shared<I32RemainderUnsigned>();
            break;
        case grammar::I32And::OPCODE:
            next = std::make_shared<I32And>();
            break;
        case grammar::I32Or::OPCODE:
            next = std::make_shared<I32Or>();
            break;
        case grammar::I32Xor::OPCODE:
            next = std::make_shared<I32Xor>();
            break;
        case grammar::I32ShiftLeft::OPCODE:
            next = std::make_shared<I32ShiftLeft>();
            break;
        case grammar::I32ShiftRightSigned::OPCODE:
            next = std::make_shared<I32ShiftRightSigned>();
            break;
        case grammar::I32ShiftRightUnsigned::OPCODE:
            next = std::make_shared<I32ShiftRightUnsigned>();
            break;
        case grammar::I32RotateLeft::OPCODE:
            next = std::make_shared<I32RotateLeft>();
            break;
        case grammar::I32RotateRight::OPCODE:
            next = std::make_shared<I32RotateRight>();
            break;
        case grammar::I64CountLeadingZeros::OPCODE:
            next = std::make_shared<I64CountLeadingZeros>();
            break;
        case grammar::I64CountTrailingZeros::OPCODE:
            next = std::make_shared<I64CountTrailingZeros>();
            break;
        case grammar::I64Add::OPCODE:
            next = std::make_shared<I64Add>();
            break;
        case grammar::I64Sub::OPCODE:
            next = std::make_shared<I64Sub>();
            break;
        case grammar::I64Mul::OPCODE:
            next = std::make_shared<I64Mul>();
            break;
        case grammar::I64DivideSigned::OPCODE:
            next = std::make_shared<I64DivideSigned>();
            break;
        case grammar::I64DivideUnsigned::OPCODE:
            next = std::make_shared<I64DivideUnsigned>();
            break;
        case grammar::I64RemainderSigned::OPCODE:
            next = std::make_shared<I64RemainderSigned>();
            break;
        case grammar::I64RemainderUnsigned::OPCODE:
            next = std::make_shared<I64RemainderUnsigned>();
            break;
        case grammar::I64And::OPCODE:
            next = std::make_shared<I64And>();
            break;
        case grammar::I64Or::OPCODE:
            next = std::make_shared<I64Or>();
            break;
        case grammar::I64Xor::OPCODE:
            next = std::make_shared<I64Xor>();
            break;
        case grammar::I64ShiftLeft::OPCODE:
            next = std::make_shared<I64ShiftLeft>();
            break;
        case grammar::I64ShiftRightSigned::OPCODE:
            next = std::make_shared<I64ShiftRightSigned>();
            break;
        case grammar::I64ShiftRightUnsigned::OPCODE:
            next = std::make_shared<I64ShiftRightUnsigned>();
            break;
        case grammar::I64RotateLeft::OPCODE:
            next = std::make_shared<I64RotateLeft>();
            break;
        case grammar::I64RotateRight::OPCODE:
            next = std::make_shared<I64RotateRight>();
            break;
        case grammar::F32Mul::OPCODE:
            next = std::make_shared<F32Mul>();
            break;
        case grammar::F32Div::OPCODE:
            next = std::make_shared<F32Div>();
            break;
        case grammar::F64Mul::OPCODE:
            next = std::make_shared<F64Mul>();
            break;
        case grammar::F64Div::OPCODE:
            next = std::make_shared<F64Div>();
            break;
        case grammar::F64CopySign::OPCODE:
            next = std::make_shared<F64CopySign>();
            break;
        case grammar::I32WrapI64::OPCODE:
            next = std::make_shared<I32WrapI64>();
            break;
        case grammar::I64ExtendI32Signed::OPCODE:
            next = std::make_shared<I64ExtendI32Signed>();
            break;
        case grammar::I64ExtendI32Unsigned::OPCODE:
            next = std::make_shared<I64ExtendI32Unsigned>();
            break;
        case grammar::F32ConvertI32Signed::OPCODE:
            next = std::make_shared<F32ConvertI32Signed>();
            break;
        case grammar::F32DemoteF64::OPCODE:
            next = std::make_shared<F32DemoteF64>();
            break;
        case grammar::F64ConvertI32Signed::OPCODE:
            next = std::make_shared<F64ConvertI32Signed>();
            break;
        case grammar::F64PromoteF32::OPCODE:
            next = std::make_shared<F64PromoteF32>();
            break;
        case grammar::I32ReinterpretF32::OPCODE:
            next = std::make_shared<I32ReinterpretF32>();
            break;
        case grammar::I64ReinterpretF64::OPCODE:
            next = std::make_shared<I64ReinterpretF64>();
            break;
        case grammar::F32ReinterpretI32::OPCODE:
            next = std::make_shared<F32ReinterpretI32>();
            break;
        case grammar::F64ReinterpretI64::OPCODE:
            next = std::make_shared<F64ReinterpretI64>();
            break;
        case grammar::I32Extend8Signed::OPCODE:
            next = std::make_shared<I32Extend8Signed>();
            break;
        case grammar::I32Extend16Signed::OPCODE:
            next = std::make_shared<I32Extend16Signed>();
            break;
        case grammar::I64Extend8Signed::OPCODE:
            next = std::make_shared<I64Extend8Signed>();
            break;
        case grammar::I64Extend16Signed::OPCODE:
            next = std::make_shared<I64Extend16Signed>();
            break;
        case grammar::I64Extend32Signed::OPCODE:
            next = std::make_shared<I64Extend32Signed>();
            break;
        // Memory Instructions
        case grammar::I32Load::OPCODE:
            next = std::make_shared<I32Load>(inst->as<grammar::I32Load>());
            break;
        case grammar::I64Load::OPCODE:
            next = std::make_shared<I64Load>(inst->as<grammar::I64Load>());
            break;
        case grammar::F32Load::OPCODE:
            next = std::make_shared<F32Load>(inst->as<grammar::F32Load>());
            break;
        case grammar::F64Load::OPCODE:
            next = std::make_shared<F64Load>(inst->as<grammar::F64Load>());
            break;
        case grammar::I32Load8Signed::OPCODE:
            next = std::make_shared<I32Load8Signed>(
                inst->as<grammar::I32Load8Signed>());
            break;
        case grammar::I32Load8Unsigned::OPCODE:
            next = std::make_shared<I32Load8Unsigned>(
                inst->as<grammar::I32Load8Unsigned>());
            break;
        case grammar::I32Load16Signed::OPCODE:
            next = std::make_shared<I32Load16Signed>(
                inst->as<grammar::I32Load16Signed>());
            break;
        case grammar::I32Load16Unsigned::OPCODE:
            next = std::make_shared<I32Load16Unsigned>(
                inst->as<grammar::I32Load16Unsigned>());
            break;
        case grammar::I64Load8Signed::OPCODE:
            next = std::make_shared<I64Load8Signed>(
                inst->as<grammar::I64Load8Signed>());
            break;
        case grammar::I64Load8Unsigned::OPCODE:
            next = std::make_shared<I64Load8Unsigned>(
                inst->as<grammar::I64Load8Unsigned>());
            break;
        case grammar::I64Load16Signed::OPCODE:
            next = std::make_shared<I64Load16Signed>(
                inst->as<grammar::I64Load16Signed>());
            break;
        case grammar::I64Load16Unsigned::OPCODE:
            next = std::make_shared<I64Load16Unsigned>(
                inst->as<grammar::I64Load16Unsigned>());
            break;
        case grammar::I64Load32Signed::OPCODE:
            next = std::make_shared<I64Load32Signed>(
                inst->as<grammar::I64Load32Signed>());
            break;
        case grammar::I64Load32Unsigned::OPCODE:
            next = std::make_shared<I64Load32Unsigned>(
                inst->as<grammar::I64Load32Unsigned>());
            break;
        case grammar::I32Store::OPCODE:
            next = std::make_shared<I32Store>(inst->as<grammar::I32Store>());
            break;
        case grammar::I64Store::OPCODE:
            next = std::make_shared<I64Store>(inst->as<grammar::I64Store>());
            break;
        case grammar::F32Store::OPCODE:
            next = std::make_shared<F32Store>(inst->as<grammar::F32Store>());
            break;
        case grammar::F64Store::OPCODE:
            next = std::make_shared<F64Store>(inst->as<grammar::F64Store>());
            break;
        case grammar::I32Store8::OPCODE:
            next = std::make_shared<I32Store8>(inst->as<grammar::I32Store8>());
            break;
        case grammar::I32Store16::OPCODE:
            next =
                std::make_shared<I32Store16>(inst->as<grammar::I32Store16>());
            break;
        case grammar::I64Store8::OPCODE:
            next = std::make_shared<I64Store8>(inst->as<grammar::I64Store8>());
            break;
        case grammar::I64Store16::OPCODE:
            next =
                std::make_shared<I64Store16>(inst->as<grammar::I64Store16>());
            break;
        case grammar::I64Store32::OPCODE:
            next =
                std::make_shared<I64Store32>(inst->as<grammar::I64Store32>());
            break;
        case grammar::MemoryGrow::OPCODE:
            next =
                std::make_shared<MemoryGrow>(inst->as<grammar::MemoryGrow>());
            break;
        case grammar::MemoryIntstructionBase::OPCODE: {
            const grammar::MemoryIntstructionBase& mem_inst =
                inst->as<grammar::MemoryIntstructionBase>();
            uint32_t mem_opcode = mem_inst.getMemoryOpcode();

            switch (mem_opcode) {
            case grammar::MemoryInit::MEM_OPCODE:
                next = std::make_shared<MemoryInit>(
                    inst->as<grammar::MemoryInit>());
                break;
            case grammar::DataDrop::MEM_OPCODE:
                next =
                    std::make_shared<DataDrop>(inst->as<grammar::DataDrop>());
                break;
            case grammar::MemoryCopy::MEM_OPCODE:
                next = std::make_shared<MemoryCopy>();
                break;
            case grammar::MemoryFill::MEM_OPCODE:
                next = std::make_shared<MemoryFill>(
                    inst->as<grammar::MemoryFill>());
                break;
            default:
                fmt::println("unknown memory opcode 0xFC 0x{:02X}", mem_opcode);
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
                next = std::make_shared<AtomicNotify>(
                    inst->as<grammar::AtomicNotify>());
                break;
            case grammar::AtomicWait32::ATOMIC_OPCODE:
                next = std::make_shared<AtomicWait32>(
                    inst->as<grammar::AtomicWait32>());
                break;
            case grammar::AtomicLoad::ATOMIC_OPCODE:
                next = std::make_shared<AtomicLoad>(
                    inst->as<grammar::AtomicLoad>());
                break;
            case grammar::AtomicLoad8Unsigned::ATOMIC_OPCODE:
                next = std::make_shared<AtomicLoad8Unsigned>(
                    inst->as<grammar::AtomicLoad8Unsigned>());
                break;
            case grammar::AtomicStore::ATOMIC_OPCODE:
                next = std::make_shared<AtomicStore>(
                    inst->as<grammar::AtomicStore>());
                break;
            case grammar::AtomicStore8::ATOMIC_OPCODE:
                next = std::make_shared<AtomicStore8>(
                    inst->as<grammar::AtomicStore8>());
                break;
            case grammar::AtomicAdd::ATOMIC_OPCODE:
                next =
                    std::make_shared<AtomicAdd>(inst->as<grammar::AtomicAdd>());
                break;
            case grammar::AtomicExchange8Unsigned::ATOMIC_OPCODE:
                next = std::make_shared<AtomicExchange8Unsigned>(
                    inst->as<grammar::AtomicExchange8Unsigned>());
                break;
            case grammar::AtomicCompareExchange::ATOMIC_OPCODE:
                next = std::make_shared<AtomicCompareExchange>(
                    inst->as<grammar::AtomicCompareExchange>());
                break;
            default:
                fmt::println("unknown atomic opcode 0xFE 0x{:02X}",
                             atomic_opcode);
                exit(1);
            }
            break;
        }
        default:
            fmt::println("unknown opcode 0x{:02X}", opcode);
            exit(1);
        }

        builder.addNext(next);
    }

    return builder.build();
}

Continuation OperationBase::trap(Instance& instance, std::string msg,
                                 size_t addr) const {
    std::string fmt_loc =
        instance.getGlobalState().getDebugInfo().getFormattedLocation(addr);
    fmt::println("trap: {}: at {}", msg, fmt_loc);
    Context& ctxt = instance.getActiveContext();
    const Operation* epilogues = ctxt.getEpilogues().data();
    for (size_t i = ctxt.getEpilogues().size() - 1; i > 0; i--) {
        const Operation& epilogue = epilogues[i];
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

void OperationBase::Builder::addNext(Operation next) {
    for (auto it = next; it; it = it->next_) {
        operations_.push_back(it);

        while (operations_.size() >= 2) {
            auto& first = operations_[operations_.size() - 2];
            auto& second = operations_[operations_.size() - 1];

            if (!canMerge(first, second))
                break;

            Operation merged = merge(first, second);

            operations_.pop_back();
            operations_.back() = merged;
        }
    }
}

Operation OperationBase::Builder::build() {
    for (size_t i = 0; i + 1 < operations_.size(); ++i) {
        operations_[i]->next_ = operations_[i + 1];
    }

    Operation start = operations_.empty() ? nullptr : operations_[0];
    operations_.clear();
    return start;
}

/**********************/
/* Control Operations */
/**********************/

Unreachable::Unreachable(const grammar::Unreachable& unreachable)
    : OperationBase(unreachable.getAddress()) {}

Continuation Unreachable::action(Instance& instance) {
    TRACE_VERBOSE("unreachable");
    return trap(instance, "unreachable instruction reached", addr_);
}

Block::Block(const grammar::Block& block,
             const std::vector<FunctionType>& func_types,
             std::vector<Operation>& targets) {
    next_ = std::make_shared<Label>(addr_);

    targets.insert(targets.begin(), next_);

    const std::vector<grammar::Instruction>& instructions =
        block.getInstruction();

    Builder builder;
    builder.addNext(OperationBase::create(instructions, func_types, targets));
    builder.addNext(next_);
    body_ = builder.build();

    targets.erase(targets.begin());
}

Continuation Block::action(Instance& instance) {
    return body_->action(instance);
}

Loop::Loop(const grammar::Loop& loop,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& labels)
    : TaggedOperation<Loop>(loop.getAddress()) {
    next_ = std::make_shared<Label>(addr_);

    Operation start = std::make_shared<Label>(addr_);
    labels.insert(labels.begin(), start);

    const std::vector<grammar::Instruction>& instructions =
        loop.getInstruction();

    Builder builder;
    builder.addNext(start);
    builder.addNext(OperationBase::create(instructions, func_types, labels));
    builder.addNext(next_);
    body_ = builder.build();

    labels.erase(labels.begin());
}

Continuation Loop::action(Instance& instance) {
    return body_->action(instance);
}

IfThen::IfThen(const grammar::IfElse& if_else,
               const std::vector<FunctionType>& func_types,
               std::vector<Operation>& labels)
    : TaggedOperation<IfThen>(if_else.getAddress()) {
    next_ = std::make_shared<Label>(addr_);

    labels.insert(labels.begin(), next_);

    Builder builder;
    builder.addNext(OperationBase::create(
        if_else.getThenExpr().getInstructions(), func_types, labels));
    builder.addNext(next_);
    then_ = builder.build();

    labels.erase(labels.begin());
}

Continuation IfThen::action(Instance& instance) {
    int32_t cond = instance.getActiveContext().pop().i32;
    return impl(*this, instance, next_.get(), cond);
}

IfElse::IfElse(const grammar::IfElse& if_else,
               const std::vector<FunctionType>& func_types,
               std::vector<Operation>& labels)
    : TaggedOperation<IfElse>(if_else.getAddress()) {
    next_ = std::make_shared<Label>(addr_);

    labels.insert(labels.begin(), next_);

    Builder builder;
    builder.addNext(OperationBase::create(
        if_else.getThenExpr().getInstructions(), func_types, labels));
    builder.addNext(next_);
    then_ = builder.build();

    builder.addNext(OperationBase::create(
        if_else.getElseExpr().getInstructions(), func_types, labels));
    builder.addNext(next_);
    else_ = builder.build();

    labels.erase(labels.begin());
}

Continuation IfElse::action(Instance& instance) {
    Value cond = instance.getActiveContext().getStack().pop();
    return impl(*this, instance, cond);
}

Branch::Branch(const grammar::Branch& branch, std::vector<Operation>& targets)
    : target_(targets[branch.getLabelIdx()]) {}

Continuation Branch::action(Instance& instance) {
    TRACE_VERBOSE("br --> {}", (void*)target_.get());
    return target_.get();
}

BranchIf::BranchIf(const grammar::BranchIf& br_if,
                   std::vector<Operation>& targets)
    : TaggedOperation<BranchIf>(br_if.getAddress()),
      target_(targets[br_if.getLabelIdx()]) {}

Continuation BranchIf::action(Instance& instance) {
    Value cond = instance.getActiveContext().pop();
    return BranchIf::impl(*this, instance, next_.get(), cond);
}

BranchTable::BranchTable(const grammar::BranchTable& br_table,
                         std::vector<Operation>& containers)
    : label_indices_(br_table.getLabelIndices()), containers_(containers) {}

Continuation BranchTable::action(Instance& instance) {
    uint32_t idx = instance.getActiveContext().pop().i32;
    if (idx >= label_indices_.size())
        idx = label_indices_.size() - 1;

    uint32_t label_idx = label_indices_[idx];
    Operation target = containers_[label_idx];

    TRACE_VERBOSE("br_table {} --> {}", label_idx, (void*)target.get());
    return target.get();
}

Call::Call(const grammar::Call& call)
    : Call(call.getFuncIdx(), call.getAddress()) {}
Call::Call(uint32_t func_idx, size_t addr)
    : TaggedOperation<Call>(addr), func_idx_(func_idx) {
    next_ = std::make_shared<Epilogue>(func_idx, addr);
}

Continuation Call::action(Instance& instance) {
    Function& func = instance.getGlobalState().getFunction(func_idx_);

    instance.getActiveContext().getEpilogues().push(next_);

    if (instance.is<Process>()) {
        TRACE("{:{}}{}: call {} is_kernel={}", "",
              instance.getActiveContext().getEpilogues().size(),
              instance.getGlobalState().getDebugInfo().getFormattedLocation(
                  addr_),
              func_idx_, instance.as<Process>().getKernel().is<Kernel>());
    }

    return func.enterFrame(instance.getActiveContext());
}

Call::Epilogue::Epilogue(uint32_t func_idx, size_t addr)
    : TaggedOperation<Epilogue>(addr), func_idx_(func_idx) {}

Continuation Call::Epilogue::action(Instance& instance) {
    if (instance.is<Process>()) {
        TRACE("{:{}}{}: ret {} is_kernel={}", "",
              instance.getActiveContext().getEpilogues().size(),
              instance.getGlobalState().getDebugInfo().getFormattedLocation(
                  addr_),
              func_idx_, instance.as<Process>().getKernel().is<Kernel>());
    }

    Function& func = instance.getGlobalState().getFunction(func_idx_);
    func.leaveFrame(instance.getActiveContext());
    return next_.get();
}

CallIndirect::CallIndirect(const grammar::CallIndirect& call_indirect,
                           const std::vector<FunctionType>& func_types)
    : TaggedOperation<CallIndirect>(call_indirect.getAddress()),
      signature_(func_types[call_indirect.getTypeIdx()].getSignature()) {
    assert(call_indirect.getTableIdx() == 0);
}

CallIndirect::CallIndirect(size_t signature) : signature_(signature) {}

Continuation CallIndirect::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t element_idx = static_cast<uint32_t>(context.pop().i32);

    std::vector<Function>& functions = instance.getGlobalState().getFunctions();
    std::vector<uint32_t>& indirections =
        instance.getGlobalState().getIndirections();

    if (element_idx >= indirections.size()) {
        return trap(
            instance,
            fmt::format("call_indirect index '{}' out of bounds", element_idx),
            addr_);
    }

    if (functions.size() < indirections[element_idx]) {
        return trap(instance,
                    fmt::format("call_indirect index '{}' out of bounds",
                                indirections[element_idx]),
                    addr_);
    }

    Function& func = functions[indirections[element_idx]];
    if (signature_ != func.getSignature())
        return trap(instance,
                    fmt::format("call_indirect type mismatch: expected '{}' "
                                "but found '{}' at element {}",

                                func.getFormattedType(), signature_,
                                element_idx),
                    addr_);

    Operation epilogue = createEpilogue(func);
    instance.getActiveContext().getEpilogues().push(epilogue);

    if (instance.is<Process>()) {
        TRACE("{:{}}{}: call_indirect: (element_idx={}) -> ()", "",
              instance.getActiveContext().getEpilogues().size(),
              instance.getGlobalState().getDebugInfo().getFormattedLocation(
                  addr_),
              element_idx);
    }

    return func.enterFrame(context);
}

const Operation& CallIndirect::createEpilogue(Function& func) {
    size_t idx;

    if (free_list_.empty()) {
        idx = epilogues_.size();
        epilogues_.push_back(std::make_shared<Epilogue>(idx, func, *this));
    } else {
        idx = free_list_.top();
        free_list_.pop();
        epilogues_[idx] = std::make_shared<Epilogue>(idx, func, *this);
    }

    return epilogues_[idx];
}

void CallIndirect::destroyEpilogue(size_t idx) {
    epilogues_[idx] = nullptr;
    free_list_.push(idx);
}

CallIndirect::Epilogue::Epilogue(size_t idx, Function& func,
                                 CallIndirect& parent)
    : idx_(idx), func_(func), parent_(parent) {}

Continuation CallIndirect::Epilogue::action(Instance& instance) {
    if (instance.is<Process>()) {
        TRACE("{:{}}{}: ret is_kernel={}", "",
              instance.getActiveContext().getEpilogues().size(),
              instance.getGlobalState().getDebugInfo().getFormattedLocation(
                  addr_),
              instance.as<Process>().getKernel().is<Kernel>());
    }

    func_.leaveFrame(instance.getActiveContext());
    parent_.destroyEpilogue(idx_);
    return parent_.next_.get();
}

Continuation Return::action(Instance& instance) {
    TRACE_VERBOSE("return");
    return nullptr;
}

/***************************/
/* Parametric Instructions */
/***************************/

Drop::Drop(const grammar::Drop& drop)
    : TaggedOperation<Drop>(drop.getAddress()) {}

Continuation Drop::action(Instance& instance) {
    instance.getActiveContext().drop();
    TRACE_VERBOSE("drop");
    return next_.get();
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
    return next_.get();
}

/*************************/
/* Variable Instructions */
/*************************/

GlobalGet::GlobalGet(const grammar::GlobalGet& global_get)
    : global_idx_(global_get.getGlobalIdx()) {}

Continuation GlobalGet::action(Instance& instance) {
    Value i = instance.getGlobalState().getGlobal(global_idx_);
    instance.getActiveContext().push(i);
    TRACE_VERBOSE("global.get {}: () -> ({})", global_idx_, i.i64);
    return next_.get();
}

GlobalSet::GlobalSet(const grammar::GlobalSet& global_set)
    : global_idx_(global_set.getGlobalIdx()) {}

Continuation GlobalSet::action(Instance& instance) {
    Value i = instance.getActiveContext().pop();
    instance.getGlobalState().setGlobal(global_idx_, i);
    TRACE_VERBOSE("global.set {}: ({}) -> ()", global_idx_, i.i64);
    return next_.get();
}

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
    context.pushI32(result);

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
    context.pushI32(result);

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
    int32_t result = left >> right;
    context.pushI32(result);

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

    uint64_t val = context.pop().i64;
    uint64_t result;

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

    uint64_t val = context.pop().i64;
    uint64_t result;

    if (val == 0)
        result = 64;
    else
        result = __builtin_ctzll(val);

    context.pushI64(result);

    TRACE_VERBOSE("i64.ctz: ({}) -> ({})", val, result);
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
    context.pushI64(result);

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
    context.pushI64(result);

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
    context.pushI64(result);

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

Continuation I32ReinterpretF32::action(Instance& instance) {
    TRACE_VERBOSE("i32.reinterpret_f32");
    return next_.get();
}

Continuation I64ReinterpretF64::action(Instance& instance) {
    TRACE_VERBOSE("i64.reinterpret_f64");
    return next_.get();
}

Continuation F32ReinterpretI32::action(Instance& instance) {
    TRACE_VERBOSE("f32.reinterpret_i32");
    return next_.get();
}

Continuation F64ReinterpretI64::action(Instance& instance) {
    TRACE_VERBOSE("f64.reinterpret_i64");
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

/***********************/
/* Memory Instructions */
/***********************/

I32Load::I32Load(const grammar::I32Load& i32_load)
    : TaggedOperation<I32Load>(i32_load.getAddress()),
      offset_(i32_load.getMemArg().getOffset()),
      align_(i32_load.getMemArg().getAlign()) {}

Continuation I32Load::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
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
            return trap(instance, "i32.load invalid memory access", addr_);
        }
    }

    context.pushI32(value);

    TRACE_VERBOSE("i32.load {} {}: ({}) -> ({})", align_, offset_, base, value);
    return next_.get();
}

I64Load::I64Load(const grammar::I64Load& i64_load)
    : offset_(i64_load.getMemArg().getOffset()),
      align_(i64_load.getMemArg().getAlign()) {}

Continuation I64Load::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
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
            context.push(base);
            context.getEpilogues().push(shared_from_this());

            // trigger page fault handling
            return mmu.fault(instance, offset, false);
        }
    }

    context.pushI64(value);

    TRACE_VERBOSE("i64.load {} {}: ({}) -> ({})", align_, offset_, base, value);
    return next_.get();
}

F32Load::F32Load(const grammar::F32Load& f32_load)
    : offset_(f32_load.getMemArg().getOffset()),
      align_(f32_load.getMemArg().getAlign()) {}

Continuation F32Load::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
    uint32_t offset = base + offset_;

    float value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "f32.load out of bounds memory address", offset);

    context.push(value);

    TRACE_VERBOSE("f32.load {} {}: ({}) -> ({})", align_, offset_, base, value);
    return next_.get();
}

F64Load::F64Load(const grammar::F64Load& f64_load)
    : offset_(f64_load.getMemArg().getOffset()),
      align_(f64_load.getMemArg().getAlign()) {}

Continuation F64Load::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
    uint32_t offset = base + offset_;

    double val;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, val))
        return trap(instance, "f64.load out of bounds memory address", offset);

    context.push(val);

    TRACE_VERBOSE("f64.load {} {}: ({}) -> ({})", align_, offset_, base, val);
    return next_.get();
}

I32Load8Signed::I32Load8Signed(const grammar::I32Load8Signed& i32_load8_s)
    : offset_(i32_load8_s.getMemArg().getOffset()),
      align_(i32_load8_s.getMemArg().getAlign()) {}

Continuation I32Load8Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
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
    return next_.get();
}

I32Load8Unsigned::I32Load8Unsigned(const grammar::I32Load8Unsigned& i32_load8_u)
    : offset_(i32_load8_u.getMemArg().getOffset()),
      align_(i32_load8_u.getMemArg().getAlign()) {}

Continuation I32Load8Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
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
            return trap(instance, "i32.load8_u invalid memory access", addr_);
        }
    }

    context.pushI32(value);
    TRACE_VERBOSE("i32.load8_u {} {}: ({}) -> ({})", align_, offset_, base,
                  value);
    return next_.get();
}

I32Load16Signed::I32Load16Signed(const grammar::I32Load16Signed& i32_load16_s)
    : offset_(i32_load16_s.getMemArg().getOffset()),
      align_(i32_load16_s.getMemArg().getAlign()) {}

Continuation I32Load16Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
    uint32_t offset = base + offset_;

    int16_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i32.load16_s out of bounds memory address",
                    offset);

    context.pushI32(value);

    TRACE_VERBOSE("i32.load16_s align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_.get();
}

I32Load16Unsigned::I32Load16Unsigned(
    const grammar::I32Load16Unsigned& i32_load16_u)
    : offset_(i32_load16_u.getMemArg().getOffset()),
      align_(i32_load16_u.getMemArg().getAlign()) {}

Continuation I32Load16Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
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
            context.push(base);
            context.getEpilogues().push(shared_from_this());

            // trigger a page fault
            return mmu.fault(instance, offset, false);
        }
    }

    context.pushI32(value);

    TRACE_VERBOSE("i32.load16_u align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_.get();
}

I64Load8Signed::I64Load8Signed(const grammar::I64Load8Signed& i64_load8_s)
    : offset_(i64_load8_s.getOffset()), align_(i64_load8_s.getAlign()) {}

Continuation I64Load8Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
    uint32_t offset = base + offset_;

    int8_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load8_s out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load8_s align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_.get();
}

I64Load8Unsigned::I64Load8Unsigned(const grammar::I64Load8Unsigned& i64_load8_u)
    : offset_(i64_load8_u.getOffset()), align_(i64_load8_u.getAlign()) {}

Continuation I64Load8Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
    uint32_t offset = base + offset_;

    uint8_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load8_u out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load8_u align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_.get();
}

I64Load16Signed::I64Load16Signed(const grammar::I64Load16Signed& i64_load16_s)
    : offset_(i64_load16_s.getOffset()), align_(i64_load16_s.getAlign()) {}

Continuation I64Load16Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
    uint32_t offset = base + offset_;

    int16_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load16_s out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load16_s align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_.get();
}

I64Load16Unsigned::I64Load16Unsigned(
    const grammar::I64Load16Unsigned& i64_load16_u)
    : offset_(i64_load16_u.getOffset()), align_(i64_load16_u.getAlign()) {}

Continuation I64Load16Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
    uint32_t offset = base + offset_;

    uint16_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load16_u out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load16_u align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_.get();
}

I64Load32Signed::I64Load32Signed(const grammar::I64Load32Signed& i64_load32_s)
    : offset_(i64_load32_s.getOffset()), align_(i64_load32_s.getAlign()) {}

Continuation I64Load32Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
    uint32_t offset = base + offset_;

    int32_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load32_s out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load32_s align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_.get();
}

I64Load32Unsigned::I64Load32Unsigned(
    const grammar::I64Load32Unsigned& i64_load32_u)
    : offset_(i64_load32_u.getOffset()), align_(i64_load32_u.getAlign()) {}

Continuation I64Load32Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.pop().i32;
    uint32_t offset = base + offset_;

    uint32_t value;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.load(offset, value))
        return trap(instance, "i64.load32_u out of bounds memory address",
                    offset);

    context.pushI64(value);

    TRACE_VERBOSE("i64.load32_u align={} offset={}: (base={}) -> (val={})",
                  align_, offset_, base, value);
    return next_.get();
}

I32Store::I32Store(const grammar::I32Store& i32_store)
    : TaggedOperation<I32Store>(i32_store.getAddress()),
      offset_(i32_store.getMemArg().getOffset()),
      align_(i32_store.getMemArg().getAlign()) {}

Continuation I32Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t value = context.pop().i32;
    int32_t base = context.pop().i32;

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
            context.pushI32(base);
            context.pushI32(value);
            context.getEpilogues().push(shared_from_this());

            if (instance.is<Kernel>()) {
                fmt::println("i32.store: {}", getFormattedAddress(instance));
            }
            // trigger page fault handling
            return mmu.fault(instance, offset, true);
        }
    }

    TRACE_VERBOSE("i32.store {} {}: ({}, {}) -> ()", align_, offset_, base,
                  value);
    return next_.get();
}

I64Store::I64Store(const grammar::I64Store& i64_store)
    : offset_(i64_store.getMemArg().getOffset()),
      align_(i64_store.getMemArg().getAlign()) {}

Continuation I64Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t value = context.pop().i64;
    int32_t base = context.pop().i32;

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
            context.pushI32(base);
            context.pushI64(value);
            context.getEpilogues().push(shared_from_this());

            // trigger page fault handling
            return mmu.fault(instance, offset, true);
        }
    }

    TRACE_VERBOSE("i64.store {} {}: ({}, {}) -> ()", align_, offset_, base,
                  value);
    return next_.get();
}

F32Store::F32Store(const grammar::F32Store& f32_store)
    : offset_(f32_store.getOffset()), align_(f32_store.getAlign()) {}

Continuation F32Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    float value = context.pop().f32;
    int32_t base = context.pop().i32;

    uint32_t offset = base + offset_;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.store(offset, value))
        return trap(instance, "f32.store out of bounds memory address", offset);

    TRACE_VERBOSE("f32.store {} {}: ({}, {}) -> ()", align_, offset_, base,
                  value);
    return next_.get();
}

F64Store::F64Store(const grammar::F64Store& f64_store)
    : offset_(f64_store.getOffset()), align_(f64_store.getAlign()) {}

Continuation F64Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double value = context.pop().f64;
    int32_t base = context.pop().i32;

    uint32_t offset = base + offset_;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.store(offset, value))
        return trap(instance, "f64.store out of bounds memory address", offset);

    TRACE_VERBOSE("f64.store {} {}: ({}, {}) -> ()", align_, offset_, base,
                  value);
    return next_.get();
}

I32Store8::I32Store8(const grammar::I32Store8& i32_store8)
    : offset_(i32_store8.getMemArg().getOffset()),
      align_(i32_store8.getMemArg().getAlign()) {}

Continuation I32Store8::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t value = context.pop().i32;
    int32_t base = context.pop().i32;

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
            context.pushI32(base);
            context.pushI32(value);
            context.getEpilogues().push(shared_from_this());

            // trigger page fault handling
            return mmu.fault(instance, offset, true);
        }
    }

    TRACE_VERBOSE("i32.store8 {} {}: ({}, {}) -> ()", align_, offset_, base,
                  value);
    return next_.get();
}

I32Store16::I32Store16(const grammar::I32Store16& i32_store16)
    : offset_(i32_store16.getMemArg().getOffset()),
      align_(i32_store16.getMemArg().getAlign()) {}

Continuation I32Store16::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t value = context.pop().i32;
    int32_t base = context.pop().i32;

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
            context.pushI32(base);
            context.pushI32(value);
            context.getEpilogues().push(shared_from_this());

            // trigger page fault handling
            return mmu.fault(instance, offset, true);
        }
    }

    TRACE_VERBOSE("i32.store16 align={} offset={}: (base={}, val={}) -> ()",
                  align_, offset_, base, value & 0xFFFF);
    return next_.get();
}

I64Store8::I64Store8(const grammar::I64Store8& i64_store8)
    : offset_(i64_store8.getMemArg().getOffset()),
      align_(i64_store8.getMemArg().getAlign()) {}

Continuation I64Store8::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t val = context.pop().i64;
    int32_t base = context.pop().i32;

    uint32_t offset = base + offset_;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.store(offset, static_cast<uint8_t>(val & 0xFF)))
        return trap(instance, "i64.store8 out of bounds memory address",
                    offset);

    TRACE_VERBOSE("i64.store8 align={} offset={}: (base={}, val={}) -> ()",
                  align_, offset_, base, val & 0xFF);
    return next_.get();
}

I64Store16::I64Store16(const grammar::I64Store16& i64_store16)
    : offset_(i64_store16.getMemArg().getOffset()),
      align_(i64_store16.getMemArg().getAlign()) {}

Continuation I64Store16::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t value = context.pop().i64;
    int32_t base = context.pop().i32;

    uint32_t offset = base + offset_;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.store(offset, static_cast<uint16_t>(value & 0xFFFF)))
        return trap(instance, "i64.store16 out of bounds memory address",
                    offset);

    TRACE_VERBOSE("i64.store16 align={} offset={}: (base={}, val={}) -> ()",
                  align_, offset_, base, value & 0xFFFF);
    return next_.get();
}

I64Store32::I64Store32(const grammar::I64Store32& i64_store32)
    : offset_(i64_store32.getMemArg().getOffset()),
      align_(i64_store32.getMemArg().getAlign()) {}

Continuation I64Store32::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t value = context.pop().i64;
    int32_t base = context.pop().i32;

    uint32_t offset = base + offset_;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.store(offset, static_cast<uint32_t>(value & 0xFFFFFFFF)))
        return trap(instance, "i64.store32 out of bounds memory address",
                    offset);

    TRACE_VERBOSE("i64.store32 align={} offset={}: (base={}, val={}) -> ()",
                  align_, offset_, base, value & 0xFFFFFFFF);
    return next_.get();
}

MemoryGrow::MemoryGrow(const grammar::MemoryGrow& mem_grow)
    : OperationBase(mem_grow.getAddress()) {}

Continuation MemoryGrow::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t delta = context.pop().i32;
    Memory& memory = instance.getGlobalState().getMemory();

    context.pushI32(memory.grow(delta));
    return next_.get();
}

MemoryInit::MemoryInit(const grammar::MemoryInit& mem_init)
    : segment_idx_(mem_init.getSegmentIdx()) {}

Continuation MemoryInit::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t len = context.pop().i32;
    int32_t src_offset = context.pop().i32;
    int32_t dst_offset = context.pop().i32;

    std::vector<uint8_t>& data_segment =
        instance.getGlobalState().getData()[segment_idx_];

    assert(src_offset + len <= data_segment.size());
    auto src_buffer = std::span<uint8_t>(data_segment).subspan(src_offset, len);

    Memory& memory = instance.getGlobalState().getMemory();
    bool result = memory.store(dst_offset, src_buffer);
    assert(result);

    return next_.get();
}

DataDrop::DataDrop(const grammar::DataDrop& data_drop)
    : segment_idx_(data_drop.getSegmentIdx()) {}

Continuation DataDrop::action(Instance& instance) {
    instance.getGlobalState().dropSegment(segment_idx_);
    return next_.get();
}

Continuation MemoryCopy::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t count = static_cast<uint32_t>(context.pop().i32);
    uint32_t src_offset = static_cast<uint32_t>(context.pop().i32);
    uint32_t dst_offset = static_cast<uint32_t>(context.pop().i32);

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
        if (!mmu.load(src_offset, buffer)) {
            // repeat the operation after handling the page fault
            context.pushI32(dst_offset);
            context.pushI32(src_offset);
            context.pushI32(count);
            context.getEpilogues().push(shared_from_this());

            // trigger page fault handling
            return mmu.fault(instance, src_offset, false);
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
        if (!mmu.store(dst_offset, buffer)) {
            // repeat the operation after handling the page fault
            context.pushI32(dst_offset);
            context.pushI32(src_offset);
            context.pushI32(count);
            context.getEpilogues().push(shared_from_this());

            // trigger page fault handling
            return mmu.fault(instance, dst_offset, true);
        }
    }

    TRACE_VERBOSE("memory.copy: (dst={}, src={}, count={})", dst_offset,
                  src_offset, count);
    return next_.get();
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
        if (!memory.fill(dst_offset, value, count))
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
        if (!mmu.fill(dst_offset, value, count)) {
            // repeat the operation after handling the page fault
            context.pushI32(dst_offset);
            context.pushI32(value);
            context.pushI32(count);
            context.getEpilogues().push(shared_from_this());

            // trigger page fault handling
            return mmu.fault(instance, dst_offset, true);
        }
    }

    return next_.get();
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
    return next_.get();
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
            return next_.get();
        }

        if (context.getRemainingTime() == 0) {
            context.setRunState(Context::RunState::running);
            context.pushI32(2);

            TRACE_VERBOSE("memory.atomic.wait32 epilogue {} {}: () -> ({})",
                          align_, offset_, 2);
            return next_.get();
        }

        return idle_.get();
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
        return next_.get();
    } else {
        waiting_room.block(context, offset);
        context.setTimeout(offset, timeout);
        context.setRunState(Context::RunState::waiting);

        // TODO: figure out what i wanted to do here?
        // idle_->addNext(shared_from_this());
        __builtin_trap();
        return idle_.get();
    }
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
    return next_.get();
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
    return next_.get();
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
    return next_.get();
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
    return next_.get();
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
    return next_.get();
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
    return next_.get();
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
    return next_.get();
}

} // namespace runtime
