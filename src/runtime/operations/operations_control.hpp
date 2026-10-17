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
    Label(size_t addr) : TaggedOperation<Label>(addr) {
        next_ = std::make_shared<Nop>(addr);
    }

    Continuation action(Instance& instance) override {
        return next_->action(instance);
    }
};

class Block : public TaggedOperation<Block> {
public:
    Block(const grammar::Block& block,
          const std::vector<FunctionType>& func_types,
          std::vector<Operation>& branch_targets,
          std::pmr::polymorphic_allocator<std::byte>& arena);

    Continuation action(Instance& instance) override;

    static OperationBase::Collector
    collect(const grammar::Block& inst, std::vector<Operation>& targets,
            std::pmr::polymorphic_allocator<std::byte>& arena);

protected:
    Block() {}

private:
    class Collector : public CollectorBase {
    public:
        Collector(const std::vector<grammar::Instruction>& instructions,
                  std::shared_ptr<Block> block, std::vector<Operation>& targets)
            : CollectorBase(instructions), block_(block), targets_(targets) {}

        Operation finish() override;

    private:
        std::shared_ptr<Block> block_;
        std::vector<Operation>& targets_;
    };

    Operation body_;
};

class Loop : public TaggedOperation<Loop> {
public:
    Loop(const grammar::Loop& loop, const std::vector<FunctionType>& func_types,
         std::vector<Operation>& branch_targets,
         std::pmr::polymorphic_allocator<std::byte>& arena);

    Continuation action(Instance& instance) override;

private:
    Operation body_;
};

class IfThen : public TaggedOperation<IfThen> {
public:
    static Continuation impl(IfThen& if_then, Instance& instance,
                             Continuation next, Value cond);

    IfThen(const grammar::IfElse& if_else,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& branch_targets,
           std::pmr::polymorphic_allocator<std::byte>& arena);

    Continuation action(Instance& instance) override;

protected:
    Operation then_;
};

class IfElse : public TaggedOperation<IfElse> {
public:
    static Continuation impl(IfElse& if_else, Instance& instance, Value cond);

    IfElse(const grammar::IfElse& if_else,
           const std::vector<FunctionType>& func_types,
           std::vector<Operation>& branch_targets,
           std::pmr::polymorphic_allocator<std::byte>& arena);

    Continuation action(Instance& instance) override;

private:
    Operation then_;
    Operation else_;
};

class Branch : public TaggedOperation<Branch> {
public:
    Branch(const grammar::Branch& branch, std::vector<Operation>& targets);

    Continuation action(Instance& instance) override;

private:
    Operation target_;
};

class BranchIf : public TaggedOperation<BranchIf> {
public:
    static Continuation impl(BranchIf& br_if, Instance& instance,
                             Continuation next, Value cond);

    BranchIf(const grammar::BranchIf& br_if, std::vector<Operation>& targets);

    Continuation action(Instance& instance) override;

private:
    Operation target_;
};

class BranchTable : public TaggedOperation<BranchTable> {
public:
    BranchTable(const grammar::BranchTable& br_table,
                std::vector<Operation>& targets);

    Continuation action(Instance& instance) override;

private:
    std::vector<uint32_t> label_indices_;
    std::vector<Operation> containers_;
};

class Call : public TaggedOperation<Call> {
public:
    Call(const grammar::Call& call,
         std::pmr::polymorphic_allocator<std::byte>& arena);
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

class Return : public TaggedOperation<Return> {
public:
    Continuation action(Instance& instance) override;
};

}
