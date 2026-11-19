#include "runtime/operations/operations.hpp"

namespace runtime {

class I32LocalSetConstBranch : public TaggedOperation<I32LocalSetConstBranch> {
public:
    static constexpr uint32_t OPCODE = 0x41'21'0C;

    I32LocalSetConstBranch(const grammar::I32Const& immediate,
                           const grammar::LocalSet& set,
                           const grammar::Branch& branch, Operations& targets)
        : TaggedOperation<I32LocalSetConstBranch>(immediate.getAddress()),
          immediate_(immediate.getVal()), local_idx_(set.getIndex()) {
        next_ = targets[branch.getLabelIdx()];
    }

    Continuation action(Instance& instance) override;

private:
    int32_t immediate_;
    uint32_t local_idx_;
};

} // namespace runtime
