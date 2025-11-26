#include <fmt/base.h>

#include "runtime/context.hpp"
#include "runtime/instance.hpp"
#include "runtime/operations.hpp"
#include "runtime/vm.hpp"

namespace runtime {

Operation
OperationBase::create(const std::vector<grammar::Instruction>& instructions,
                      const std::vector<FunctionType>& func_types,
                      std::vector<Operation>& targets) {
    Operation op;

    for (size_t i = 0; i < instructions.size(); ++i) {
        Operation next;

        const grammar::Instruction& instruction = instructions[i];
        uint8_t opcode = instruction->getOpcode();

        switch (opcode) {
        // Control Instructions
        case grammar::Unreachable::OPCODE:
            next = std::make_shared<Unreachable>(
                instruction->as<grammar::Unreachable>());
            break;
        case grammar::Nop::OPCODE:
            next = std::make_shared<Nop>();
            break;
        case grammar::Block::OPCODE:
            next = std::make_shared<Block>(instruction->as<grammar::Block>(),
                                           func_types, targets);
            break;
        case grammar::Loop::OPCODE:
            next = std::make_shared<Loop>(instruction->as<grammar::Loop>(),
                                          func_types, targets);
            break;
        case grammar::Branch::OPCODE:
            next = std::make_shared<Branch>(instruction->as<grammar::Branch>(),
                                            targets);
            break;
        case grammar::BranchIf::OPCODE:
            next = std::make_shared<BranchIf>(
                instruction->as<grammar::BranchIf>(), targets);
            break;
        case grammar::BranchTable::OPCODE:
            next = std::make_shared<BranchTable>(
                instruction->as<grammar::BranchTable>(), targets);
            break;
        case grammar::Call::OPCODE:
            next = std::make_shared<Call>(instruction->as<grammar::Call>());
            break;
        case grammar::CallIndirect::OPCODE:
            next = std::make_shared<CallIndirect>(
                instruction->as<grammar::CallIndirect>(), func_types);
            break;
        case grammar::Return::OPCODE:
            next = std::make_shared<Return>();
            break;
        // Parametric Instructions
        case grammar::Drop::OPCODE:
            next = std::make_shared<Drop>(instruction->as<grammar::Drop>());
            break;
        case grammar::Select::OPCODE:
            next = std::make_shared<Select>();
            break;
        // Variable Instruction
        case grammar::LocalGet::OPCODE:
            next = std::make_shared<LocalGet>(
                instruction->as<grammar::LocalGet>());
            break;
        case grammar::LocalSet::OPCODE:
            next = std::make_shared<LocalSet>(
                instruction->as<grammar::LocalSet>());
            break;
        case grammar::GlobalGet::OPCODE:
            next = std::make_shared<GlobalGet>(
                instruction->as<grammar::GlobalGet>());
            break;
        case grammar::GlobalSet::OPCODE:
            next = std::make_shared<GlobalSet>(
                instruction->as<grammar::GlobalSet>());
            break;
        // Numeric Instructions
        case grammar::I32Const::OPCODE:
            next = std::make_shared<I32Const>(
                instruction->as<grammar::I32Const>());
            break;
        case grammar::I64Const::OPCODE:
            next = std::make_shared<I64Const>(
                instruction->as<grammar::I64Const>());
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
        case grammar::I32CountLeadingZeros::OPCODE:
            next = std::make_shared<I32CountLeadingZeros>();
            break;
        case grammar::I32CountTrailingZeros::OPCODE:
            next = std::make_shared<I32CountTrailingZeros>();
            break;
        case grammar::I32Add::OPCODE:
            next = std::make_shared<I32Add>();
            break;
        case grammar::I32Sub::OPCODE:
            next = std::make_shared<I32Sub>();
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
        case grammar::I32WrapI64::OPCODE:
            next = std::make_shared<I32WrapI64>();
            break;
        case grammar::I64ExtendI32Signed::OPCODE:
            next = std::make_shared<I64ExtendI32Signed>();
            break;
        case grammar::I64ExtendI32Unsigned::OPCODE:
            next = std::make_shared<I64ExtendI32Unsigned>();
            break;
        // Memory Instructions
        case grammar::I32Load::OPCODE:
            next =
                std::make_shared<I32Load>(instruction->as<grammar::I32Load>());
            break;
        case grammar::I64Load::OPCODE:
            next =
                std::make_shared<I64Load>(instruction->as<grammar::I64Load>());
            break;
        case grammar::I32Load8Signed::OPCODE:
            next = std::make_shared<I32Load8Signed>(
                instruction->as<grammar::I32Load8Signed>());
            break;
        case grammar::I32Load8Unsigned::OPCODE:
            next = std::make_shared<I32Load8Unsigned>(
                instruction->as<grammar::I32Load8Unsigned>());
            break;
        case grammar::I32Load16Unsigned::OPCODE:
            next = std::make_shared<I32Load16Unsigned>(
                instruction->as<grammar::I32Load16Unsigned>());
            break;
        case grammar::I64Load32Unsigned::OPCODE:
            next = std::make_shared<I64Load32Unsigned>(
                instruction->as<grammar::I64Load32Unsigned>());
            break;
        case grammar::I32Store::OPCODE:
            next = std::make_shared<I32Store>(
                instruction->as<grammar::I32Store>());
            break;
        case grammar::I64Store::OPCODE:
            next = std::make_shared<I64Store>(
                instruction->as<grammar::I64Store>());
            break;
        case grammar::F64Store::OPCODE:
            next = std::make_shared<F64Store>(
                instruction->as<grammar::F64Store>());
            break;
        case grammar::I32Store8::OPCODE:
            next = std::make_shared<I32Store8>(
                instruction->as<grammar::I32Store8>());
            break;
        case grammar::I32Store16::OPCODE:
            next = std::make_shared<I32Store16>(
                instruction->as<grammar::I32Store16>());
            break;
        case grammar::MemoryGrow::OPCODE:
            next = std::make_shared<MemoryGrow>(
                instruction->as<grammar::MemoryGrow>());
            break;
        case grammar::MemoryIntstructionBase::OPCODE: {
            const grammar::MemoryIntstructionBase& mem_inst =
                instruction->as<grammar::MemoryIntstructionBase>();
            uint32_t mem_opcode = mem_inst.getMemoryOpcode();

            switch (mem_opcode) {
            case grammar::MemoryInit::MEM_OPCODE:
                next = std::make_shared<MemoryInit>(
                    instruction->as<grammar::MemoryInit>());
                break;
            case grammar::DataDrop::MEM_OPCODE:
                next = std::make_shared<DataDrop>(
                    instruction->as<grammar::DataDrop>());
                break;
            case grammar::MemoryCopy::MEM_OPCODE:
                next = std::make_shared<MemoryCopy>();
                break;
            case grammar::MemoryFill::MEM_OPCODE:
                next = std::make_shared<MemoryFill>();
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
                instruction->as<grammar::AtomicIntstructionBase>();
            uint32_t atomic_opcode = atomic_inst.getAtomicOpcode();

            switch (atomic_opcode) {
            case grammar::AtomicNotify::ATOMIC_OPCODE:
                next = std::make_shared<AtomicNotify>(
                    instruction->as<grammar::AtomicNotify>());
                break;
            case grammar::AtomicWait32::ATOMIC_OPCODE:
                next = std::make_shared<AtomicWait32>(
                    instruction->as<grammar::AtomicWait32>());
                break;
            case grammar::AtomicLoad::ATOMIC_OPCODE:
                next = std::make_shared<AtomicLoad>(
                    instruction->as<grammar::AtomicLoad>());
                break;
            case grammar::AtomicLoad8Unsigned::ATOMIC_OPCODE:
                next = std::make_shared<AtomicLoad8Unsigned>(
                    instruction->as<grammar::AtomicLoad8Unsigned>());
                break;
            case grammar::AtomicStore::ATOMIC_OPCODE:
                next = std::make_shared<AtomicStore>(
                    instruction->as<grammar::AtomicStore>());
                break;
            case grammar::AtomicStore8::ATOMIC_OPCODE:
                next = std::make_shared<AtomicStore8>(
                    instruction->as<grammar::AtomicStore8>());
                break;
            case grammar::AtomicAdd::ATOMIC_OPCODE:
                next = std::make_shared<AtomicAdd>(
                    instruction->as<grammar::AtomicAdd>());
                break;
            case grammar::AtomicExchange8Unsigned::ATOMIC_OPCODE:
                next = std::make_shared<AtomicExchange8Unsigned>(
                    instruction->as<grammar::AtomicExchange8Unsigned>());
                break;
            case grammar::AtomicCompareExchange::ATOMIC_OPCODE:
                next = std::make_shared<AtomicCompareExchange>(
                    instruction->as<grammar::AtomicCompareExchange>());
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

        if (op)
            op->addNext(next);
        else
            op = next;
    }

    return op;
}

Continuation OperationBase::trap(Instance& instance, std::string msg,
                                 size_t addr) const {
    std::string fmt_loc =
        instance.getGlobalState().getDebugInfo().getFormattedLocation(addr);
    fmt::println("trap: {} (at {})", msg, fmt_loc);
    std::exit(1);
    return nullptr;
}

/*********************/
/* Helper Operations */
/*********************/

Continuation Tick::action(Instance& instance) {
    const Operation& epilogue = createEpilogue(instance);

    instance.switchToKernel();
    instance.getKernel().getActiveContext().pushReturn(epilogue.get());

    return instance.tick();
}

Tick::Epilogue::Epilogue(size_t idx, Instance& suspended_instance, Tick& parent)
    : idx_(idx), suspended_instance_(suspended_instance), parent_(parent) {}

Continuation Tick::Epilogue::action(Instance& instance) {
    instance.switchToInstance(suspended_instance_);
    return parent_.next_.get();
}

const Operation& Tick::createEpilogue(Instance& instance) {
    size_t idx;

    if (free_list_.empty()) {
        idx = epilogues_.size();
        epilogues_.push_back(std::make_shared<Epilogue>(idx, instance, *this));
    } else {
        idx = free_list_.top();
        free_list_.pop();
        epilogues_[idx] = std::make_shared<Epilogue>(idx, instance, *this);
    }

    return epilogues_[idx];
}

void Tick::destroyEpilogue(size_t idx) {
    epilogues_[idx] = nullptr;
    free_list_.push(idx);
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
    targets.insert(targets.begin(), end_);

    const std::vector<grammar::Instruction>& instructions =
        block.getInstruction();
    start_ = OperationBase::create(instructions, func_types, targets);
    start_->addNext(end_);

    targets.erase(targets.begin());
}

Continuation Block::action(Instance& instance) {
    TRACE_VERBOSE("block[{}]", (void*)end_.get());
    return start_.get();
}

Loop::Loop(const grammar::Loop& loop,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& targets) {
    targets.insert(targets.begin(), start_);

    const std::vector<grammar::Instruction>& instructions =
        loop.getInstruction();

    Operation body = OperationBase::create(instructions, func_types, targets);
    body->addNext(end_);

    start_->addNext(body);
    targets.erase(targets.begin());
}

Continuation Loop::action(Instance& instance) {
    TRACE_VERBOSE("loop[{}]", (void*)start_.get());
    return start_.get();
}

Branch::Branch(const grammar::Branch& branch, std::vector<Operation>& targets)
    : target_(targets[branch.getLabelIdx()]) {}

Continuation Branch::action(Instance& instance) {
    TRACE_VERBOSE("br --> {}", (void*)target_.get());
    return target_.get();
}

BranchIf::BranchIf(const grammar::BranchIf& br_if,
                   std::vector<Operation>& targets)
    : target_(targets[br_if.getLabelIdx()]) {}

Continuation BranchIf::action(Instance& instance) {
    int32_t cond = instance.getActiveContext().popI32();
    TRACE_VERBOSE("br_if --> {}: (cond={}) -> ()", (void*)target_.get(), cond);

    if (cond)
        return target_.get();

    return next_.get();
}

BranchTable::BranchTable(const grammar::BranchTable& br_table,
                         std::vector<Operation>& containers)
    : label_indices_(br_table.getLabelIndices()), containers_(containers) {}

Continuation BranchTable::action(Instance& instance) {
    uint32_t idx = instance.getActiveContext().popI32();
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
    : OperationBase(addr), func_idx_(func_idx),
      epilogue_(std::make_shared<Epilogue>(func_idx)) {}

Continuation Call::action(Instance& instance) {
    Function& func = instance.getGlobalState().getFunction(func_idx_);

    instance.getActiveContext().pushReturn(epilogue_.get());

    TRACE(
        "{}: call {}",
        instance.getGlobalState().getDebugInfo().getFormattedLocation(addr_),
        func_idx_);
    return func.enterFrame(instance.getActiveContext());
}

Call::Epilogue::Epilogue(uint32_t func_idx) : func_idx_(func_idx) {}

Continuation Call::Epilogue::action(Instance& instance) {
    Function& func = instance.getGlobalState().getFunction(func_idx_);
    func.leaveFrame(instance.getActiveContext());
    return next_.get();
}

CallIndirect::CallIndirect(const grammar::CallIndirect& call_indirect,
                           const std::vector<FunctionType>& func_types)
    : signature_(func_types[call_indirect.getTypeIdx()].getSignature()) {
    assert(call_indirect.getTableIdx() == 0);
}

CallIndirect::CallIndirect(size_t signature) : signature_(signature) {}

Continuation CallIndirect::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t element_idx = static_cast<uint32_t>(context.popI32());
    Function& func = instance.getGlobalState().getFunctionIndirect(element_idx);

    if (signature_ != func.getSignature())
        return trap(instance,
                    "call_indirect type mismatch: expected {} but found {} at "
                    "element {}",
                    addr_);

    Operation epilogue = createEpilogue(func);
    instance.getActiveContext().pushReturn(epilogue.get());

    TRACE_VERBOSE("call_indirect: (element_idx={}) -> ()", element_idx);
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

Drop::Drop(const grammar::Drop& drop) : OperationBase(drop.getAddress()) {}

Continuation Drop::action(Instance& instance) {
    instance.getActiveContext().drop();
    TRACE_VERBOSE("drop");
    return next_.get();
}

Continuation Select::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t cond = context.popI32();
    Value b = context.pop();
    Value a = context.pop();
    Value result = cond ? a : b;
    context.push(result);

    TRACE_VERBOSE("select: (a={}, b={}, cond={}) -> ({})",
              (a.index() == 0) ? std::get<int32_t>(a) : std::get<int64_t>(a),
              (b.index() == 0) ? std::get<int32_t>(b) : std::get<int64_t>(b),
              cond,
              (result.index() == 0) ? std::get<int32_t>(result)
                                    : std::get<int64_t>(result));
    return next_.get();
}

/*************************/
/* Variable Instructions */
/*************************/

LocalGet::LocalGet(const grammar::LocalGet& local_get)
    : OperationBase(local_get.getAddress()),
      local_idx_(local_get.getLocalIdx()) {}

Continuation LocalGet::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    Value i = context.getLocal(local_idx_);
    context.push(i);
    TRACE_VERBOSE(
        "{}: local.get {}: () -> ({})",
        instance.getGlobalState().getDebugInfo().getFormattedLocation(addr_),
        local_idx_,
        i.index() == 0 ? std::get<int32_t>(i) : std::get<int64_t>(i));
    return next_.get();
}

LocalSet::LocalSet(const grammar::LocalSet& local_set)
    : local_idx_(local_set.getLocalIdx()) {}

Continuation LocalSet::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    Value i = context.pop();
    context.setLocal(local_idx_, i);
    TRACE_VERBOSE("local.set {}: ({}) -> ()", local_idx_,
              i.index() == 0 ? std::get<int32_t>(i) : std::get<int64_t>(i));
    return next_.get();
}

GlobalGet::GlobalGet(const grammar::GlobalGet& global_get)
    : global_idx_(global_get.getGlobalIdx()) {}

Continuation GlobalGet::action(Instance& instance) {
    Value i = instance.getGlobalState().getGlobal(global_idx_);
    instance.getActiveContext().push(i);
    TRACE_VERBOSE("global.get {}: () -> ({})", global_idx_,
              i.index() == 0 ? std::get<int32_t>(i) : std::get<int64_t>(i));
    return next_.get();
}

GlobalSet::GlobalSet(const grammar::GlobalSet& global_set)
    : global_idx_(global_set.getGlobalIdx()) {}

Continuation GlobalSet::action(Instance& instance) {
    Value i = instance.getActiveContext().pop();
    instance.getGlobalState().setGlobal(global_idx_, i);
    TRACE_VERBOSE("global.set {}: ({}) -> ()", global_idx_,
              i.index() == 0 ? std::get<int32_t>(i) : std::get<int64_t>(i));
    return next_.get();
}

/************************/
/* Numeric Instructions */
/************************/

I32Const::I32Const(const grammar::I32Const& i32_const)
    : i_(i32_const.getVal()) {}

Continuation I32Const::action(Instance& instance) {
    instance.getActiveContext().pushI32(i_);

    TRACE_VERBOSE("i32.const {}: () -> ({})", i_, i_);
    return next_.get();
}

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

Continuation I32EqualZero::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.popI32();
    int32_t result = val == 0 ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.eqz: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I32Equal::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = (left == right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.eq: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32NotEqual::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = (left != right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.ne: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32LessThanSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = (left < right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.lt_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32LessThanUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.popI32());
    uint32_t left = static_cast<uint32_t>(context.popI32());
    int32_t result = (left < right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.lt_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32GreaterThanSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = (left > right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.gt_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32GreaterThanUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.popI32());
    uint32_t left = static_cast<uint32_t>(context.popI32());
    int32_t result = (left > right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.gt_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32LessEqualSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = (left <= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.ge_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32LessEqualUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.popI32());
    uint32_t left = static_cast<uint32_t>(context.popI32());
    int32_t result = (left <= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.ge_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32GreaterEqualSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = (left >= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.ge_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32GreaterEqualUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.popI32());
    uint32_t left = static_cast<uint32_t>(context.popI32());
    int32_t result = (left >= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i32.ge_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64EqualZero::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t val = context.popI64();
    int32_t result = val == 0 ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.eqz: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I64Equal::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int32_t result = (left == right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.eq: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64NotEqual::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int32_t result = (left != right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.ne: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64LessThanSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int32_t result = (left < right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.lt_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64LessThanUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t right = static_cast<uint64_t>(context.popI64());
    uint64_t left = static_cast<uint64_t>(context.popI64());
    int32_t result = (left < right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.lt_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64GreaterThanSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int32_t result = (left > right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.gt_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64GreaterThanUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t right = static_cast<uint64_t>(context.popI64());
    uint64_t left = static_cast<uint64_t>(context.popI64());
    int32_t result = (left > right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.gt_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64LessEqualSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int32_t result = (left <= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.le_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64LessEqualUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t right = static_cast<uint64_t>(context.popI64());
    uint64_t left = static_cast<uint64_t>(context.popI64());
    int32_t result = (left <= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.le_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64GreaterEqualSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int32_t result = (left >= right) ? 1 : 0;
    context.pushI32(result);

    TRACE_VERBOSE("i64.ge_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32CountLeadingZeros::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t val = context.popI32();
    uint32_t result;

    if (val == 0)
        result = 32;
    else
        result = __builtin_clz(val);

    context.pushI32(result);

    TRACE_VERBOSE("i32.clz: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I32CountTrailingZeros::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t val = context.popI32();
    uint32_t result;

    if (val == 0)
        result = 32;
    else
        result = __builtin_ctz(val);

    context.pushI32(result);

    TRACE_VERBOSE("i32.ctz: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I32Add::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = left + right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.add: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32Sub::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = left - right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.sub: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32Mul::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = left * right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.mul: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32DivideSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    if (right == 0)
        return trap(instance, "i32.div_s: division by zero", addr_);

    int32_t left = context.popI32();
    if (left == std::numeric_limits<int32_t>::min() && right == -1)
        return trap(instance, "i32.div_s: integer overflow (INT32_MIN / -1)",
                    addr_);

    int32_t result = left / right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.div_s: (left={}, right={}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32DivideUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.popI32());
    if (right == 0)
        return trap(instance, "i32.div_u: division by zero", addr_);

    uint32_t left = static_cast<uint32_t>(context.popI32());
    uint32_t result = left / right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.div_u: (left={}, right={}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32RemainderSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    if (right == 0)
        return trap(instance, "i32.div_s: division by zero", addr_);

    int32_t left = context.popI32();
    int32_t result = left % right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.rem_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32RemainderUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t right = static_cast<uint32_t>(context.popI32());
    if (right == 0)
        return trap(instance, "i32.div_u: division by zero", addr_);

    uint32_t left = static_cast<uint32_t>(context.popI32());
    uint32_t result = left % right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.rem_u: (left={}, right={}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32And::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = left & right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.and: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32Or::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = left | right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.or: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32Xor::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32();
    int32_t left = context.popI32();
    int32_t result = left ^ right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.xor: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32ShiftLeft::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32() & 31;
    int32_t left = context.popI32();
    int32_t result = left << right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.shl: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32ShiftRightSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32() & 31;
    int32_t left = context.popI32();
    int32_t result = left >> right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.shr_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32ShiftRightUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t right = context.popI32() & 31;
    uint32_t left = static_cast<uint32_t>(context.popI32());
    int32_t result = left >> right;
    context.pushI32(result);

    TRACE_VERBOSE("i32.shr_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64CountLeadingZeros::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t val = context.popI64();
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

    uint64_t val = context.popI64();
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

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int64_t result = left + right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.add: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64Sub::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int64_t result = left - right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.sub: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64Mul::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int64_t result = left * right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.mul: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64DivideSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    if (right == 0)
        return trap(instance, "i64.div_s: division by zero", addr_);

    int64_t left = context.popI64();
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

    uint64_t right = static_cast<uint64_t>(context.popI64());
    if (right == 0)
        return trap(instance, "i64.div_u: division by zero", addr_);

    uint64_t left = static_cast<uint64_t>(context.popI64());
    uint64_t result = left / right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.div_u: (left={}, right={}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64RemainderSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    if (right == 0)
        return trap(instance, "i64.rem_s: division by zero", addr_);

    int64_t left = context.popI64();
    int64_t result = left % right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.rem_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64RemainderUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint64_t right = static_cast<uint64_t>(context.popI64());
    if (right == 0)
        return trap(instance, "i64.rem_u: division by zero", addr_);

    uint64_t left = static_cast<uint64_t>(context.popI64());
    uint64_t result = left % right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.rem_u: (left={}, right={}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64And::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int64_t result = left & right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.and: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64Or::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int64_t result = left | right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.or: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64Xor::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64();
    int64_t left = context.popI64();
    int64_t result = left ^ right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.xor: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64ShiftLeft::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64() & 63;
    int64_t left = context.popI64();
    int64_t result = left << right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.shl: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64ShiftRightSigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64() & 63;
    int64_t left = context.popI64();
    int64_t result = left >> right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.shr_s: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I64ShiftRightUnsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t right = context.popI64() & 63;
    uint64_t left = static_cast<uint64_t>(context.popI64());
    uint64_t result = left >> right;
    context.pushI64(result);

    TRACE_VERBOSE("i64.shr_u: ({}, {}) -> ({})", left, right, result);
    return next_.get();
}

Continuation I32WrapI64::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t val = context.popI64();
    int32_t result = static_cast<int32_t>(val);
    context.pushI32(result);

    TRACE_VERBOSE("i32.wrap_i64: ({}) -> ({})", val, result);
    return next_.get();
}

Continuation I64ExtendI32Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.popI32();
    context.pushI64(val);

    TRACE_VERBOSE("i64.extend_i32_s: ({}) -> ({})", val, val);
    return next_.get();
}

Continuation I64ExtendI32Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t val = static_cast<uint32_t>(context.popI32());
    context.pushI64(val);

    TRACE_VERBOSE("extend_i32_u: ({}) -> ({})", val, val);
    return next_.get();
}

/***********************/
/* Memory Instructions */
/***********************/

I32Load::I32Load(const grammar::I32Load& i32_load)
    : offset_(i32_load.getMemArg().getOffset()),
      align_(i32_load.getMemArg().getAlign()) {}

Continuation I32Load::action(Instance& instance) {
    int32_t base = instance.getActiveContext().popI32();

    uint32_t addr = base + offset_;
    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int32_t) > memory.size())
        return trap(instance, "i32.load out of bounds memory address", addr);

    int32_t val = *reinterpret_cast<int32_t*>(&memory[addr]);
    instance.getActiveContext().pushI32(val);

    TRACE_VERBOSE("i32.load {} {}: ({}) -> ({})", align_, offset_, base, val);
    return next_.get();
}

I64Load::I64Load(const grammar::I64Load& i64_load)
    : offset_(i64_load.getMemArg().getOffset()),
      align_(i64_load.getMemArg().getAlign()) {}

Continuation I64Load::action(Instance& instance) {
    int32_t base = instance.getActiveContext().popI32();

    uint32_t addr = base + offset_;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int64_t) > memory.size())
        return trap(instance, "i64.load out of bounds memory address", addr);

    int64_t val = *reinterpret_cast<int64_t*>(&memory[addr]);
    instance.getActiveContext().pushI64(val);

    TRACE_VERBOSE("i64.load {} {}: ({}) -> ({})", align_, offset_, base, val);
    return next_.get();
}

I32Load8Signed::I32Load8Signed(const grammar::I32Load8Signed& i32_load8_s)
    : offset_(i32_load8_s.getMemArg().getOffset()),
      align_(i32_load8_s.getMemArg().getAlign()) {}

Continuation I32Load8Signed::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.popI32();
    uint32_t addr = base + offset_;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int8_t) > memory.size())
        return trap(instance, "i32.load8_s out of bounds memory address", addr);

    int8_t val = static_cast<int8_t>(memory[addr]);
    context.pushI32(val);

    TRACE_VERBOSE("i32.load8_s {} {}: ({}) -> ({})", align_, offset_, base, val);
    return next_.get();
}

I32Load8Unsigned::I32Load8Unsigned(const grammar::I32Load8Unsigned& i32_load8_u)
    : offset_(i32_load8_u.getMemArg().getOffset()),
      align_(i32_load8_u.getMemArg().getAlign()) {}

Continuation I32Load8Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.popI32();
    uint32_t addr = base + offset_;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int8_t) > memory.size())
        return trap(instance, "i32.load8_u out of bounds memory address", addr);

    uint8_t val = memory[addr];
    context.pushI32(val);

    TRACE_VERBOSE("i32.load8_u {} {}: ({}) -> ({})", align_, offset_, base, val);
    return next_.get();
}

I32Load16Unsigned::I32Load16Unsigned(
    const grammar::I32Load16Unsigned& i32_load16_u)
    : offset_(i32_load16_u.getMemArg().getOffset()),
      align_(i32_load16_u.getMemArg().getAlign()) {}

Continuation I32Load16Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.popI32();
    uint32_t addr = base + offset_;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(uint16_t) > memory.size())
        return trap(instance, "i32.load16_u out of bounds memory address",
                    addr);

    uint16_t val = *reinterpret_cast<uint16_t*>(&memory[addr]);
    context.pushI32(val);

    TRACE_VERBOSE("i32.load16_u align={} offset={}: (base={}) -> (val={})", align_,
              offset_, base, val);
    return next_.get();
}

I64Load32Unsigned::I64Load32Unsigned(
    const grammar::I64Load32Unsigned& i64_load32_u)
    : offset_(i64_load32_u.getOffset()), align_(i64_load32_u.getAlign()) {}

Continuation I64Load32Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t base = context.popI32();
    uint32_t addr = base + offset_;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(uint32_t) > memory.size())
        return trap(instance, "i64.load32_u out of bounds memory address",
                    addr);

    uint32_t val = *reinterpret_cast<uint32_t*>(&memory[addr]);
    context.pushI64(val);

    TRACE_VERBOSE("i64.load32_u align={} offset={}: (base={}) -> (val={})", align_,
              offset_, base, val);
    return next_.get();
}

I32Store::I32Store(const grammar::I32Store& i32_store)
    : offset_(i32_store.getMemArg().getOffset()),
      align_(i32_store.getMemArg().getAlign()) {}

Continuation I32Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.popI32();
    int32_t base = context.popI32();

    uint32_t addr = base + offset_;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int32_t) > memory.size())
        return trap(instance, "i32.store out of bounds memory address", addr);

    *reinterpret_cast<int32_t*>(&memory[addr]) = val;

    TRACE_VERBOSE("i32.store {} {}: ({}, {}) -> ()", align_, offset_, base, val);
    return next_.get();
}

I64Store::I64Store(const grammar::I64Store& i64_store)
    : offset_(i64_store.getMemArg().getOffset()),
      align_(i64_store.getMemArg().getAlign()) {}

Continuation I64Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int64_t val = context.popI64();
    int32_t base = context.popI32();

    uint32_t addr = base + offset_;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int64_t) > memory.size())
        return trap(instance, "i64.store out of bounds memory address", addr);

    *reinterpret_cast<int64_t*>(&memory[addr]) = val;

    TRACE_VERBOSE("i64.store {} {}: ({}, {}) -> ()", align_, offset_, base, val);
    return next_.get();
}

F64Store::F64Store(const grammar::F64Store& f64_store)
    : offset_(f64_store.getOffset()), align_(f64_store.getAlign()) {}

Continuation F64Store::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    double val = context.popF64();
    int32_t base = context.popI32();

    uint32_t addr = base + offset_;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(double) > memory.size())
        return trap(instance, "f64.store out of bounds memory address", addr);

    *reinterpret_cast<double*>(&memory[addr]) = val;

    TRACE_VERBOSE("f64.store {} {}: ({}, {}) -> ()", align_, offset_, base, val);
    return next_.get();
}

I32Store8::I32Store8(const grammar::I32Store8& i32_store8)
    : offset_(i32_store8.getMemArg().getOffset()),
      align_(i32_store8.getMemArg().getAlign()) {}

Continuation I32Store8::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.popI32();
    int32_t base = context.popI32();

    uint32_t addr = base + offset_;
    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int8_t) > memory.size())
        return trap(instance, "i32.store8 out of bounds memory address", addr);

    memory[addr] = static_cast<uint8_t>(val & 0xFF);

    TRACE_VERBOSE("i32.store8 {} {}: ({}, {}) -> ()", align_, offset_, base, val);
    return next_.get();
}

I32Store16::I32Store16(const grammar::I32Store16& i32_store16)
    : offset_(i32_store16.getMemArg().getOffset()),
      align_(i32_store16.getMemArg().getAlign()) {}

Continuation I32Store16::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.popI32();
    int32_t base = context.popI32();

    uint32_t addr = base + offset_;
    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int16_t) > memory.size())
        return trap(instance, "i32.store16 out of bounds memory address", addr);

    *reinterpret_cast<uint16_t*>(&memory[addr]) =
        static_cast<uint16_t>(val & 0xFFFF);

    TRACE_VERBOSE("i32.store16 align={} offset={}: (base={}, val={}) -> ()", align_,
              offset_, base, val & 0xFFFF);
    return next_.get();
}

MemoryGrow::MemoryGrow(const grammar::MemoryGrow& mem_grow)
    : OperationBase(mem_grow.getAddress()) {}

Continuation MemoryGrow::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t delta = context.popI32();
    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();

    constexpr uint32_t PAGE_SIZE = 65536;

    uint32_t old_pages = memory.size() / PAGE_SIZE;
    if (delta == 0) {
        context.pushI32(old_pages);
        return next_.get();
    }

    uint64_t new_size = static_cast<uint64_t>(memory.size()) +
                        static_cast<uint64_t>(delta) * PAGE_SIZE;
    if (new_size > UINT32_MAX) {
        context.pushI32(-1);
        return next_.get();
    }

    memory.resize(new_size, 0);
    context.pushI32(old_pages);
    return next_.get();
}

MemoryInit::MemoryInit(const grammar::MemoryInit& mem_init)
    : segment_idx_(mem_init.getSegmentIdx()) {}

Continuation MemoryInit::action(Instance& instance) {
    int32_t len = instance.getActiveContext().popI32();
    int32_t src = instance.getActiveContext().popI32();
    int32_t dst = instance.getActiveContext().popI32();

    /* TODO this check should be done at instantiation */
    std::vector<uint8_t>& data_segment =
        instance.getGlobalState().getData()[segment_idx_];
    if (src + len > data_segment.size())
        return trap(instance, "memory.init out of bounds data segment address",
                    addr_);

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (dst + len > memory.size())
        return trap(instance, "memory.init out of bounds memory address",
                    addr_);

    memcpy(&memory[dst], &data_segment[src], len);
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

    uint32_t count = static_cast<uint32_t>(context.popI32());
    uint32_t src = static_cast<uint32_t>(context.popI32());
    uint32_t dst = static_cast<uint32_t>(context.popI32());

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (src + count > memory.size() || dst + count > memory.size())
        return trap(instance, "memory.copy out of bounds memory address",
                    addr_);

    memmove(&memory[dst], &memory[src], count);

    TRACE_VERBOSE("memory.copy: (dst={}, src={}, count={})", dst, src, count);
    return next_.get();
}

Continuation MemoryFill::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t count = static_cast<uint32_t>(context.popI32());
    int32_t val = context.popI32();
    uint32_t dst = static_cast<uint32_t>(context.popI32());

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (dst + count > memory.size())
        return trap(instance, "memory.fill out of bounds memory address",
                    addr_);

    memset(&memory[dst], val, count);
    return next_.get();
};

/******************************/
/* Atomic Memory Instructions */
/******************************/

AtomicNotify::AtomicNotify(const grammar::AtomicNotify& atomic_notify)
    : offset_(atomic_notify.getMemArg().getOffset()),
      align_(atomic_notify.getMemArg().getAlign()) {}

Continuation AtomicNotify::action(Instance& instance) {
    uint32_t waiters =
        static_cast<uint32_t>(instance.getActiveContext().popI32());
    uint32_t base = static_cast<uint32_t>(instance.getActiveContext().popI32());

    uint32_t addr = base + offset_;
    if (addr % align_ != 0)
        return trap(instance, "memory.atomic.notify misaligned memory address",
                    addr_);

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int32_t) > memory.size())
        return trap(instance,
                    "memory.atomic.notify out of bounds memory address", addr_);

    WaitingRoom& waiting_room = instance.getWaitingRoom();
    uint32_t notified = waiting_room.notify(addr, waiters);

    instance.getActiveContext().pushI32(static_cast<int32_t>(notified));
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

            TRACE_VERBOSE("memory.atomic.wait32 epilogue {} {}: () -> ({})", align_,
                      offset_, 0);
            return next_.get();
        }

        if (context.getRemainingTime() == 0) {
            context.setRunState(Context::RunState::running);
            context.pushI32(2);

            TRACE_VERBOSE("memory.atomic.wait32 epilogue {} {}: () -> ({})", align_,
                      offset_, 2);
            return next_.get();
        }

        return idle_.get();
    }

    int64_t timeout = context.popI64();
    int32_t expected = context.popI32();
    uint32_t base = static_cast<uint32_t>(context.popI32());

    uint32_t addr = base + offset_;
    if (addr % align_ != 0)
        return trap(instance, "memory.atomic.wait32 misaligned memory address",
                    addr_);

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int32_t) > memory.size())
        return trap(instance,
                    "memory.atomic.wait32 out of bounds memory address", addr_);

    int32_t val = __atomic_load_n(reinterpret_cast<int32_t*>(&memory[addr]),
                                  __ATOMIC_SEQ_CST);

    TRACE_VERBOSE("memory.atomic.wait32 prologue {} {}: ({}, {}, {}) -> ()", align_,
              offset_, timeout, expected, base);

    if (val != expected) {
        context.pushI32(1);

        TRACE_VERBOSE("memory.atomic.wait32 epilogue {} {}: () -> ({})", align_,
                  offset_, 1);
        return next_.get();
    } else {
        waiting_room.block(context, addr);
        context.setTimeout(addr, timeout);
        context.setRunState(Context::RunState::waiting);

        idle_->addNext(shared_from_this());
        return idle_.get();
    }
}

AtomicLoad::AtomicLoad(const grammar::AtomicLoad& atomic_load)
    : offset_(atomic_load.getMemArg().getOffset()),
      align_(atomic_load.getMemArg().getAlign()) {}

Continuation AtomicLoad::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.popI32());

    uint32_t addr = base + offset_;
    if (addr % align_ != 0)
        return trap(instance, "memory.atomic.load misaligned memory address",
                    addr_);

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int32_t) > memory.size())
        return trap(instance, "memory.atomic.load out of bounds memory address",
                    addr_);

    int32_t* ptr = reinterpret_cast<int32_t*>(&memory[addr]);
    int32_t val = __atomic_load_n(ptr, __ATOMIC_SEQ_CST);

    context.pushI32(val);

    TRACE_VERBOSE("memory.atomic.load {} {}: ({}) -> ({})", align_, offset_, base,
              val);
    return next_.get();
}

AtomicLoad8Unsigned::AtomicLoad8Unsigned(
    const grammar::AtomicLoad8Unsigned& atomic_load8_u)
    : offset_(atomic_load8_u.getMemArg().getOffset()),
      align_(atomic_load8_u.getMemArg().getAlign()) {}

Continuation AtomicLoad8Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t base = static_cast<uint32_t>(context.popI32());

    uint32_t addr = base + offset_;
    if (addr % align_ != 0)
        return trap(instance, "memory.atomic.load8_u misaligned memory address",
                    addr_);

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(uint8_t) > memory.size())
        return trap(instance, "memory.atomic.load8_u misaligned memory address",
                    addr_);

    uint8_t* ptr = &memory[addr];
    uint8_t val = __atomic_load_n(ptr, __ATOMIC_SEQ_CST);

    context.pushI32(val);

    TRACE_VERBOSE("memory.atomic.load8_u {} {}: ({}) -> ({})", align_, offset_,
              base, val);
    return next_.get();
}

AtomicStore::AtomicStore(const grammar::AtomicStore& atomic_store)
    : offset_(atomic_store.getMemArg().getOffset()),
      align_(atomic_store.getMemArg().getAlign()) {}

Continuation AtomicStore::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.popI32();
    uint32_t base = static_cast<uint32_t>(context.popI32());

    uint32_t addr = base + offset_;
    if (addr % align_ != 0)
        return trap(instance, "memory.atomic.store misaligned memory address",
                    addr_);

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(int32_t) > memory.size())
        return trap(instance,
                    "memory.atomic.store out of bounds memory address", addr_);

    int32_t* ptr = reinterpret_cast<int32_t*>(&memory[addr]);
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);

    TRACE_VERBOSE("memory.atomic.store {} {}: ({}, {}) -> ()", align_, offset_, val,
              base);
    return next_.get();
}

AtomicStore8::AtomicStore8(const grammar::AtomicStore8& atomic_store8)
    : offset_(atomic_store8.getMemArg().getOffset()),
      align_(atomic_store8.getMemArg().getAlign()) {}

Continuation AtomicStore8::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint8_t val = static_cast<uint8_t>(context.popI32());
    uint32_t base = static_cast<uint32_t>(context.popI32());

    uint32_t addr = base + offset_;
    if (addr % align_ != 0)
        return trap(instance, "memory.atomic.store8 misaligned memory address",
                    addr_);

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(uint8_t) > memory.size())
        return trap(instance,
                    "memory.atomic.store8 out of bounds memory address", addr_);

    uint8_t* ptr = &memory[addr];
    __atomic_store_n(ptr, val, __ATOMIC_SEQ_CST);

    TRACE_VERBOSE("memory.atomic.store8 {} {}: ({}, {}) -> ()", align_, offset_,
              val, base);
    return next_.get();
}

AtomicAdd::AtomicAdd(const grammar::AtomicAdd& atomic_add)
    : offset_(atomic_add.getMemArg().getOffset()),
      align_(atomic_add.getMemArg().getAlign()) {}

Continuation AtomicAdd::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    int32_t val = context.popI32();
    uint32_t base = static_cast<uint32_t>(context.popI32());

    uint32_t addr = base + offset_;
    if (addr % align_ != 0)
        return trap(instance, "i32.atomic.rmw.add misaligned memory address",
                    addr_);

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + 4 > memory.size())
        return trap(instance, "i32.atomic.rmw.add out of bounds memory address",
                    addr_);

    uint32_t* ptr = reinterpret_cast<uint32_t*>(&memory[addr]);
    uint32_t old =
        __atomic_fetch_add(ptr, static_cast<uint32_t>(val), __ATOMIC_SEQ_CST);

    context.pushI32(static_cast<int32_t>(old));

    TRACE_VERBOSE("i32.atomic.rmw.add {} {}: ({}, {}) -> ()", align_, offset_, base,
              val, old);
    return next_.get();
}

AtomicExchange8Unsigned::AtomicExchange8Unsigned(
    const grammar::AtomicExchange8Unsigned& atomic_xchg8_u)
    : offset_(atomic_xchg8_u.getMemArg().getOffset()),
      align_(atomic_xchg8_u.getMemArg().getAlign()) {}

Continuation AtomicExchange8Unsigned::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint8_t val = static_cast<uint8_t>(context.popI32());
    uint32_t base = static_cast<uint32_t>(context.popI32());

    uint32_t addr = base + offset_;
    if (addr % align_ != 0)
        return trap(instance,
                    "i32.atomic.rmw8.xchg_u misaligned memory address", addr_);

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (addr + sizeof(uint8_t) > memory.size())
        return trap(instance,
                    "i32.atomic.rmw8.xchg_u out of bonds memory address",
                    addr_);

    uint8_t* ptr = &memory[addr];
    uint8_t old = __atomic_exchange_n(ptr, val, __ATOMIC_SEQ_CST);
    context.pushI32(old);

    TRACE_VERBOSE("i32.atomic.rmw8.xchg_u {} {}: ({}, {}) -> ({})", align_, offset_,
              val, base, old);
    return next_.get();
}

AtomicCompareExchange::AtomicCompareExchange(
    const grammar::AtomicCompareExchange& atomic_cmpxchg)
    : offset_(atomic_cmpxchg.getMemArg().getOffset()),
      align_(atomic_cmpxchg.getMemArg().getAlign()) {}

Continuation AtomicCompareExchange::action(Instance& instance) {
    int32_t desired = instance.getActiveContext().popI32();
    int32_t expected = instance.getActiveContext().popI32();
    uint32_t base = static_cast<uint32_t>(instance.getActiveContext().popI32());

    uint32_t offset = base + offset_;
    if (offset % align_ != 0)
        return trap(instance,
                    "i32.atomic.rmw.cmpxchg misaligned memory address", addr_);

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (offset + sizeof(int32_t) > memory.size())
        return trap(instance,
                    "i32.atomic.rmw.cmpxchg out of bounds memory address",
                    addr_);

    int32_t* addr = reinterpret_cast<int32_t*>(&memory[offset]);
    __atomic_compare_exchange_n(addr, &expected, desired, false,
                                __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);

    instance.getActiveContext().pushI32(expected);
    return next_.get();
}

} // namespace runtime
