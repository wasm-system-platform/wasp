#include "runtime/operations/operations_control.hpp"
#include "runtime/instance.hpp"
#include "runtime/operations/operations_control_impl.hpp"

namespace runtime {

/**********************/
/* Control Operations */
/**********************/

Unreachable::Unreachable(const grammar::Unreachable& unreachable)
    : OperationBase(unreachable.getAddress()) {}

Continuation Unreachable::action(Instance& instance) {
    TRACE_VERBOSE("unreachable");
    return trap(instance, "unreachable instruction reached", addr_);
}

Continuation Block::action(Instance& instance) {
    return body_->action(instance);
}

OperationBase::Collector
Block::collect(const grammar::Block& inst, Operations& targets,
               std::pmr::polymorphic_allocator<std::byte>& arena) {
    class Builder : public Block {
    public:
        Builder() {}
    };

    auto block = arena_new<Builder>(arena);
    block->next_ = arena_new<Label>(arena, block->addr_, arena);
    targets.insert(targets.begin(), block->next_);

    return std::make_unique<Collector>(inst.getInstruction(), block, targets);
}

Operation Block::Collector::finish() {
    add(block_->as<Block>().next_);
    block_->as<Block>().body_ = do_finish();
    targets_.erase(targets_.begin());
    return block_;
}

Loop::Loop(const grammar::Loop& loop,
           const std::vector<FunctionType>& func_types, Operations& targets,
           std::pmr::polymorphic_allocator<std::byte>& arena)
    : TaggedOperation<Loop>(loop.getAddress()) {
    next_ = arena_new<Label>(arena, addr_, arena);

    Operation start = arena_new<Label>(arena, addr_, arena);
    targets.insert(targets.begin(), start);

    const std::vector<grammar::Instruction>& instructions =
        loop.getInstruction();

    CollectorBase collector({});
    collector.add(start);
    collector.add(
        OperationBase::create(instructions, func_types, targets, arena));
    collector.add(next_);
    body_ = collector.finish();

    targets.erase(targets.begin());
}

Continuation Loop::action(Instance& instance) {
    return body_->action(instance);
}

IfThen::IfThen(const grammar::IfElse& if_else,
               const std::vector<FunctionType>& func_types, Operations& targets,
               std::pmr::polymorphic_allocator<std::byte>& arena)
    : TaggedOperation<IfThen>(if_else.getAddress()) {
    next_ = arena_new<Label>(arena, addr_, arena);

    targets.insert(targets.begin(), next_);

    const std::vector<grammar::Instruction>& instructions =
        if_else.getThenExpr().getInstructions();

    CollectorBase collector({});
    collector.add(
        OperationBase::create(instructions, func_types, targets, arena));
    collector.add(next_);
    then_ = collector.finish();

    targets.erase(targets.begin());
}

Continuation IfThen::action(Instance& instance) {
    int32_t cond = instance.getActiveContext().pop().i32;
    return impl(*this, instance, next_, cond);
}

IfElse::IfElse(const grammar::IfElse& if_else,
               const std::vector<FunctionType>& func_types, Operations& targets,
               std::pmr::polymorphic_allocator<std::byte>& arena)
    : TaggedOperation<IfElse>(if_else.getAddress()) {
    next_ = arena_new<Label>(arena, addr_, arena);

    targets.insert(targets.begin(), next_);

    CollectorBase collector({});
    collector.add(OperationBase::create(if_else.getThenExpr().getInstructions(),
                                        func_types, targets, arena));
    collector.add(next_);
    then_ = collector.finish();

    collector.add(OperationBase::create(if_else.getElseExpr().getInstructions(),
                                        func_types, targets, arena));
    collector.add(next_);
    else_ = collector.finish();

    targets.erase(targets.begin());
}

Continuation IfElse::action(Instance& instance) {
    Value cond = instance.getActiveContext().getStack().pop();
    return impl(*this, instance, cond);
}

Branch::Branch(const grammar::Branch& branch, Operations& targets)
    : target_(targets[branch.getLabelIdx()]) {}

Continuation Branch::action(Instance&) {
    TRACE_VERBOSE("br --> {}", (void*)target_.get());
    return target_;
}

BranchIf::BranchIf(const grammar::BranchIf& br_if, Operations& targets)
    : TaggedOperation<BranchIf>(br_if.getAddress()),
      target_(targets[br_if.getLabelIdx()]) {}

Continuation BranchIf::action(Instance& instance) {
    Value cond = instance.getActiveContext().pop();
    return BranchIf::impl(*this, instance, next_, cond);
}

BranchTable::BranchTable(const grammar::BranchTable& br_table,
                         Operations& containers)
    : label_indices_(br_table.getLabelIndices()), targets_(containers) {}

Continuation BranchTable::action(Instance& instance) {
    uint32_t idx = static_cast<uint32_t>(instance.getActiveContext().pop().i32);
    if (idx >= label_indices_.size())
        idx = static_cast<uint32_t>(label_indices_.size()) - 1;

    uint32_t label_idx = label_indices_[idx];
    Continuation target = targets_[label_idx];

    TRACE_VERBOSE("br_table {} --> {}", label_idx, (void*)target.get());
    return target;
}

Call::Call(const grammar::Call& call)
    : Call(call.getFuncIdx(), call.getAddress()) {}
Call::Call(uint32_t func_idx, size_t addr)
    : TaggedOperation<Call>(addr), func_idx_(func_idx),
      epilogue_(func_idx, addr) {
    next_ = &epilogue_;
}

Continuation Call::action(Instance& instance) {
    Function& func = instance.getGlobalState().getFunction(func_idx_);
    instance.getActiveContext().getEpilogues().push(next_);

    TRACE("[{:3}] {}: call {} (sp=0x{:x})",
          instance.getActiveContext().getEpilogues().size(),
          instance.getGlobalState().getDebugInfo().getFormattedLocation(addr_),
          func_idx_, (uint32_t)instance.getGlobalState().getGlobal(0).i32);

    return func.enterFrame(instance.getActiveContext());
}

Call::Epilogue::Epilogue(uint32_t func_idx, size_t addr)
    : TaggedOperation<Epilogue>(addr), func_idx_(func_idx) {}

Continuation Call::Epilogue::action(Instance& instance) {
    TRACE("[{:3}] {}: ret {}",
          instance.getActiveContext().getEpilogues().size(),
          instance.getGlobalState().getDebugInfo().getFormattedLocation(addr_),
          func_idx_);

    Function& func = instance.getGlobalState().getFunction(func_idx_);
    func.leaveFrame(instance.getActiveContext());
    return next_;
}

CallIndirect::CallIndirect(const grammar::CallIndirect& call_indirect,
                           const std::vector<FunctionType>& func_types)
    : TaggedOperation<CallIndirect>(call_indirect.getAddress()),
      signature_(func_types[call_indirect.getTypeIdx()].getSignature()),
      epilogue_(*this) {
    assert(call_indirect.getTableIdx() == 0);
}

CallIndirect::CallIndirect(size_t signature)
    : signature_(signature), epilogue_(*this) {}

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

    uint32_t func_idx = indirections[element_idx];
    Function& func = functions[func_idx];
    if (signature_ != func.getSignature())
        return trap(instance,
                    fmt::format("call_indirect type mismatch: expected '{}' "
                                "but found '{}' at element {}",

                                func.getFormattedType(), signature_,
                                element_idx),
                    addr_);

    Context& ctxt = instance.getActiveContext();

    ctxt.getEpilogues().push(&epilogue_);
    ctxt.pushIndirectCall(func_idx);

    TRACE("[{:3}] {}: call_indirect: (element_idx={}) -> ()",
          instance.getActiveContext().getEpilogues().size(),
          instance.getGlobalState().getDebugInfo().getFormattedLocation(addr_),
          element_idx);

    return func.enterFrame(context);
}

Continuation CallIndirect::Epilogue::action(Instance& instance) {
    TRACE("[{:3}] {}: ret", instance.getActiveContext().getEpilogues().size(),
          instance.getGlobalState().getDebugInfo().getFormattedLocation(addr_));

    Context& ctxt = instance.getActiveContext();

    std::vector<Function>& functions = instance.getGlobalState().getFunctions();
    uint32_t func_idx = ctxt.popIndirectCall();
    Function& func = functions[func_idx];

    func.leaveFrame(instance.getActiveContext());
    return parent_.next_;
}

Continuation Return::action(Instance&) {
    TRACE_VERBOSE("return");
    return nullptr;
}

} // namespace runtime
