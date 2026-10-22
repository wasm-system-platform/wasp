#pragma once

#include "runtime/operations/operations.hpp"

namespace runtime {

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
    Label(size_t addr, std::pmr::polymorphic_allocator<std::byte>& arena)
        : TaggedOperation<Label>(addr) {
        next_ = arena_new<Nop>(arena, addr);
    }

    Continuation action(Instance& instance) override {
        return next_->action(instance);
    }
};

class Block : public TaggedOperation<Block> {
public:
    Continuation action(Instance& instance) override;

    static OperationBase::Collector
    collect(const grammar::Block& inst, Operations& targets,
            std::pmr::polymorphic_allocator<std::byte>& arena);

protected:
    Block() {}

private:
    class Collector : public CollectorBase {
    public:
        Collector(const std::vector<grammar::Instruction>& instructions,
                  Operation block, Operations& targets)
            : CollectorBase(instructions), block_(block), targets_(targets) {}

        Operation finish() override;

    private:
        Operation block_;
        Operations& targets_;
    };

    Operation body_;
};

class Loop : public TaggedOperation<Loop> {
public:
    Loop(const grammar::Loop& loop, const std::vector<FunctionType>& func_types,
         Operations& targets,
         std::pmr::polymorphic_allocator<std::byte>& arena);

    Continuation action(Instance& instance) override;

private:
    Operation body_;
};

class IfThen : public TaggedOperation<IfThen> {
public:
    static Continuation impl(IfThen& if_then, Instance& instance,
                             Operation next, Value cond);

    IfThen(const grammar::IfElse& if_else,
           const std::vector<FunctionType>& func_types, Operations& targets,
           std::pmr::polymorphic_allocator<std::byte>& arena);

    Continuation action(Instance& instance) override;

protected:
    Operation then_;
};

class IfElse : public TaggedOperation<IfElse> {
public:
    static Continuation impl(IfElse& if_else, Instance& instance, Value cond);

    IfElse(const grammar::IfElse& if_else,
           const std::vector<FunctionType>& func_types, Operations& targets,
           std::pmr::polymorphic_allocator<std::byte>& arena);

    Continuation action(Instance& instance) override;

private:
    Operation then_;
    Operation else_;
};

class Branch : public TaggedOperation<Branch> {
public:
    Branch(const grammar::Branch& branch, Operations& targets);

    Continuation action(Instance& instance) override;

private:
    Operation target_;
};

class BranchIf : public TaggedOperation<BranchIf> {
public:
    static Continuation impl(BranchIf& br_if, Instance& instance,
                             Continuation next, Value cond);

    BranchIf(const grammar::BranchIf& br_if, Operations& targets);

    Continuation action(Instance& instance) override;

private:
    Operation target_;
};

class BranchTable : public TaggedOperation<BranchTable> {
public:
    BranchTable(const grammar::BranchTable& br_table, Operations& targets);

    Continuation action(Instance& instance) override;

private:
    std::vector<uint32_t> label_indices_;
    Operations targets_;
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
    Epilogue epilogue_;
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
        Epilogue(CallIndirect& parent) : parent_(parent) {}

        Continuation action(Instance& instance) override;

        CallIndirect& parent_;
    };

    size_t signature_;
    Epilogue epilogue_;
};

class Return : public TaggedOperation<Return> {
public:
    Continuation action(Instance& instance) override;
};

} // namespace runtime
