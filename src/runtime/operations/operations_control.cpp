#include "runtime/instance.hpp"
#include "runtime/operations/operations_control.hpp"
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

Block::Block(const grammar::Block& block,
             const std::vector<FunctionType>& func_types,
             std::vector<Operation>& targets,
             std::pmr::polymorphic_allocator<std::byte>& arena) {
    fmt::println("oof");
    next_ = std::pmr::make_shared<Label>(arena, addr_);

    targets.insert(targets.begin(), next_);

    const std::vector<grammar::Instruction>& instructions =
        block.getInstruction();

    CollectorBase collector(instructions);
    collector.add(
        OperationBase::create(instructions, func_types, targets, arena));
    collector.add(next_);
    body_ = collector.finish();

    targets.erase(targets.begin());
}

Continuation Block::action(Instance& instance) {
    return body_->action(instance);
}

OperationBase::Collector
Block::collect(const grammar::Block& inst, std::vector<Operation>& targets,
               std::pmr::polymorphic_allocator<std::byte>& arena) {
    class Builder : public Block {
    public:
        Builder() {}
    };

    auto block = std::pmr::make_shared<Builder>(arena);
    block->next_ = std::pmr::make_shared<Label>(arena, block->addr_);
    targets.insert(targets.begin(), block->next_);

    return std::make_unique<Collector>(inst.getInstruction(), block, targets);
}

Operation Block::Collector::finish() {
    add(block_->next_);
    block_->body_ = do_finish();
    targets_.erase(targets_.begin());
    return block_;
}

Loop::Loop(const grammar::Loop& loop,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& labels,
           std::pmr::polymorphic_allocator<std::byte>& arena)
    : TaggedOperation<Loop>(loop.getAddress()) {
    next_ = std::pmr::make_shared<Label>(arena, addr_);

    Operation start = std::pmr::make_shared<Label>(arena, addr_);
    labels.insert(labels.begin(), start);

    const std::vector<grammar::Instruction>& instructions =
        loop.getInstruction();

    CollectorBase collector({});
    collector.add(start);
    collector.add(
        OperationBase::create(instructions, func_types, labels, arena));
    collector.add(next_);
    body_ = collector.finish();

    labels.erase(labels.begin());
}

Continuation Loop::action(Instance& instance) {
    return body_->action(instance);
}

IfThen::IfThen(const grammar::IfElse& if_else,
               const std::vector<FunctionType>& func_types,
               std::vector<Operation>& labels,
               std::pmr::polymorphic_allocator<std::byte>& arena)
    : TaggedOperation<IfThen>(if_else.getAddress()) {
    next_ = std::pmr::make_shared<Label>(arena, addr_);

    labels.insert(labels.begin(), next_);

    const std::vector<grammar::Instruction>& instructions =
        if_else.getThenExpr().getInstructions();

    CollectorBase collector({});
    collector.add(
        OperationBase::create(instructions, func_types, labels, arena));
    collector.add(next_);
    then_ = collector.finish();

    labels.erase(labels.begin());
}

Continuation IfThen::action(Instance& instance) {
    int32_t cond = instance.getActiveContext().pop().i32;
    return impl(*this, instance, next_.get(), cond);
}

IfElse::IfElse(const grammar::IfElse& if_else,
               const std::vector<FunctionType>& func_types,
               std::vector<Operation>& labels,
               std::pmr::polymorphic_allocator<std::byte>& arena)
    : TaggedOperation<IfElse>(if_else.getAddress()) {
    next_ = std::pmr::make_shared<Label>(arena, addr_);

    labels.insert(labels.begin(), next_);

    CollectorBase collector({});
    collector.add(OperationBase::create(if_else.getThenExpr().getInstructions(),
                                        func_types, labels, arena));
    collector.add(next_);
    then_ = collector.finish();

    collector.add(OperationBase::create(if_else.getElseExpr().getInstructions(),
                                        func_types, labels, arena));
    collector.add(next_);
    else_ = collector.finish();

    labels.erase(labels.begin());
}

Continuation IfElse::action(Instance& instance) {
    Value cond = instance.getActiveContext().getStack().pop();
    return impl(*this, instance, cond);
}

Branch::Branch(const grammar::Branch& branch, std::vector<Operation>& targets)
    : target_(targets[branch.getLabelIdx()]) {}

Continuation Branch::action(Instance&) {
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
    uint32_t idx = static_cast<uint32_t>(instance.getActiveContext().pop().i32);
    if (idx >= label_indices_.size())
        idx = static_cast<uint32_t>(label_indices_.size()) - 1;

    uint32_t label_idx = label_indices_[idx];
    Operation target = containers_[label_idx];

    TRACE_VERBOSE("br_table {} --> {}", label_idx, (void*)target.get());
    return target.get();
}

Call::Call(const grammar::Call& call,
           std::pmr::polymorphic_allocator<std::byte>& arena)
    : TaggedOperation<Call>(call.getAddress()), func_idx_(call.getFuncIdx()) {
    next_ = std::pmr::make_shared<Epilogue>(arena, call.getFuncIdx(),
                                            call.getAddress());
}

Call::Call(uint32_t func_idx, size_t addr)
    : TaggedOperation<Call>(addr), func_idx_(func_idx) {
    next_ = std::make_shared<Epilogue>(func_idx, addr);
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

    TRACE("[{:3}] {}: call_indirect: (element_idx={}) -> ()",
          instance.getActiveContext().getEpilogues().size(),
          instance.getGlobalState().getDebugInfo().getFormattedLocation(addr_),
          element_idx);

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
    TRACE("[{:3}] {}: ret", instance.getActiveContext().getEpilogues().size(),
          instance.getGlobalState().getDebugInfo().getFormattedLocation(addr_));

    func_.leaveFrame(instance.getActiveContext());
    parent_.destroyEpilogue(idx_);
    return parent_.next_.get();
}

Continuation Return::action(Instance&) {
    TRACE_VERBOSE("return");
    return nullptr;
}

}
