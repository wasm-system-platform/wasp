#include <map>

#include "runtime/instance.hpp"
#include "runtime/operations_impl.hpp"
#include "runtime/optimization.hpp"
#include "util/hash.hpp"

namespace runtime {

/**********************/
/* Generic Operations */
/**********************/

template <size_t num_in>
class GenericConsumer : public TaggedOperation<GenericConsumer<num_in>> {
public:
    Continuation action(Instance& instance) override {
        std::array<Value, num_in> in;
        instance.getActiveContext().getStack().pop(in);
        do_impl(instance, in);
        return OperationBase::next_.get();
    }

private:
    virtual void do_impl(Instance& instance, const std::array<Value, num_in>& in) = 0;
};

template <size_t num_out>
class GenericProducer : public TaggedOperation<GenericProducer<num_out>> {
public:
    Continuation action(Instance& instance) override {
        std::array<Value, num_out> out = do_impl(instance);
        instance.getActiveContext().getStack().push(out);
        return OperationBase::next_.get();
    }

private:
    virtual std::array<Value, num_out> do_impl(Instance& instance) = 0;

    template <size_t> friend class GenericProducer_LocalGet;
};

template <size_t num_in, size_t num_out>
class GenericPipe : public TaggedOperation<GenericPipe<num_in, num_out>> {
public:
    Continuation action(Instance& instance) override {
        Stack& stack = instance.getActiveContext().getStack();

        std::array<Value, num_in> in;
        stack.pop(in);

        std::array<Value, num_out> out = do_impl(instance, in);
        stack.push(out);

        return OperationBase::next_.get();
    }

private:
    virtual std::array<Value, num_out>
    do_impl(Instance& instance, const std::array<Value, num_in>& in) = 0;
};

template <size_t num_out>
class GenericProducer_LocalGet : public GenericProducer<num_out + 1> {
public:
    GenericProducer_LocalGet(Operation&& producer, Operation&& local_get)
        : producer_(std::move(producer)), local_get_(std::move(local_get)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<GenericProducer<num_out>>());
        assert(second->is<LocalGet>());
        return std::make_shared<GenericProducer_LocalGet>(std::move(first), std::move(second));
    }

private:
    Operation producer_;
    Operation local_get_;

    std::array<Value, num_out + 1> do_impl(Instance& instance) override {
        std::array<Value, num_out + 1> out;

        auto produced =
            producer_->as<GenericProducer<num_out>>().do_impl(instance);
        std::copy(produced.begin(), produced.end(), out.begin());

        out[num_out] = LocalGet::impl(local_get_->as<LocalGet>(), instance);

        return out;
    }
};

/*************************/
/* Variable Instructions */
/*************************/

class LocalGet_LocalGet : public TaggedOperation<LocalGet_LocalGet> {
public:
    LocalGet_LocalGet(Operation&& local_get1, Operation&& local_get2)
        : local_get1_(std::move(local_get1)), local_get2_(std::move(local_get2)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<LocalGet>());
        assert(second->is<LocalGet>());
        return std::make_shared<LocalGet_LocalGet>(std::move(first), std::move(second));
    }

    Continuation action(Instance& instance) override {
        auto [local1, local2] = impl(*this, instance);
        Context& context = instance.getActiveContext();
        context.push(local1);
        context.push(local2);
        return next_.get();
    }

private:
    Operation local_get1_;
    Operation local_get2_;

    inline static std::array<Value, 2>
    impl(LocalGet_LocalGet& local_get_local_get_, Instance& instance) {
        return {LocalGet::impl(local_get_local_get_.local_get1_->as<LocalGet>(), instance),
                LocalGet::impl(local_get_local_get_.local_get2_->as<LocalGet>(), instance)};
    }
};

class LocalGet_I32Const : public TaggedOperation<LocalGet_I32Const> {
public:
    LocalGet_I32Const(Operation&& local_get, Operation&& i32_const)
        : local_get_(std::move(local_get)), i32_const_(std::move(i32_const)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<LocalGet>());
        assert(second->is<I32Const>());
        return std::make_shared<LocalGet_I32Const>(std::move(first), std::move(second));
    }

    Continuation action(Instance& instance) override {
        auto out = impl(*this, instance);
        instance.getActiveContext().getStack().push(out);
        return next_.get();
    }

private:
    Operation local_get_;
    Operation i32_const_;

    inline static std::array<Value, 2>
    impl(LocalGet_I32Const& local_get_i32_const, Instance& instance) {
        return {LocalGet::impl(local_get_i32_const.local_get_->as<LocalGet>(), instance),
                I32Const::impl(local_get_i32_const.i32_const_->as<I32Const>(), instance)};
    }

    friend class LocalGet_I32Const_I32Add;
};

/*
class LocalGet_I32Const_I32Add : public GenericProducer<1> {
public:
    LocalGet_I32Const_I32Add(Operation&& local_get_i32_const, Operation&& i32_add)
        : local_get_i32_const_(std::move(local_get_i32_const)), i32_add_(std::move(i32_add)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<LocalGet_I32Const>());
        assert(second->is<I32Add>());
        return std::make_shared<LocalGet_I32Const_I32Add>(std::move(first), std::move(second));
    }

private:
    Operation local_get_i32_const_;
    Operation i32_add_;

    std::array<Value, 1> do_impl(Instance& instance) override {
        //I32Add::impl();
        LocalGet_I32Const::impl(local_get_i32_const_->as<LocalGet_I32Const>(), instance);
    }
};
*/

class LocalSet_LocalGet : public TaggedOperation<LocalSet_LocalGet> {
public:
    LocalSet_LocalGet(Operation&& local_set, Operation&& local_get)
        : local_set_(std::move(local_set)), local_get_(std::move(local_get)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<LocalSet>());
        assert(second->is<LocalGet>());
        return std::make_shared<LocalSet_LocalGet>(std::move(first), std::move(second));
    }

    Continuation action(Instance& instance) override {
        Stack& stack = instance.getActiveContext().getStack();
        Value in = stack.pop();
        Value out = impl(*this, instance, in);
        stack.push(out);
        return next_.get();
    }

private:
    Operation local_set_;
    Operation local_get_;

    inline static Value impl(LocalSet_LocalGet& local_get_local_get_,
                             Instance& instance, Value in) {
        LocalSet::impl(local_get_local_get_.local_set_->as<LocalSet>(), instance, in);
        return LocalGet::impl(local_get_local_get_.local_get_->as<LocalGet>(), instance);
    }

    friend class LocalSet_LocalGet_LocalGet;
};

class LocalSet_LocalGet_LocalGet : public GenericPipe<1, 2> {
public:
    LocalSet_LocalGet_LocalGet(Operation&& local_set_local_get,
                               Operation&& local_get)
        : local_set_local_get_(std::move(local_set_local_get)), local_get_(std::move(local_get)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<LocalSet_LocalGet>());
        assert(second->is<LocalGet>());
        return std::make_shared<LocalSet_LocalGet_LocalGet>(std::move(first),
                                                            std::move(second));
    }

private:
    Operation local_set_local_get_;
    Operation local_get_;

    std::array<Value, 2> do_impl(Instance& instance,
                                 const std::array<Value, 1>& in) override {
        return {LocalSet_LocalGet::impl(local_set_local_get_->as<LocalSet_LocalGet>(), instance, in[0]),
                LocalGet::impl(local_get_->as<LocalGet>(), instance)};
    }
};

class LocalSet_I32Const : public TaggedOperation<LocalSet_I32Const> {
public:
    LocalSet_I32Const(Operation&& local_set, Operation&& i32_const)
        : local_set_(std::move(local_set)), i32_const_(std::move(i32_const)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<LocalSet>());
        assert(second->is<I32Const>());
        return std::make_shared<LocalSet_I32Const>(std::move(first), std::move(second));
    }

    Continuation action(Instance& instance) override {
        Stack& stack = instance.getActiveContext().getStack();
        Value in = stack.pop();
        Value out = impl(*this, instance, in);
        stack.push(out);
        return next_.get();
    }

private:
    Operation local_set_;
    Operation i32_const_;

    inline static Value impl(LocalSet_I32Const& local_set_i32_const, Instance& instance, Value in) {
        LocalSet::impl(local_set_i32_const.local_set_->as<LocalSet>(), instance, in);
        return I32Const::impl(local_set_i32_const.i32_const_->as<I32Const>(), instance);
    }

    friend class LocalSet_I32Const_LocalSet;
};

class LocalSet_I32Const_LocalSet : public GenericConsumer<1> {
public:
    LocalSet_I32Const_LocalSet(Operation&& local_set_i32_const, Operation&& local_set)
        : local_set_i32_const_(std::move(local_set_i32_const)), local_set_(std::move(local_set)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<LocalSet_I32Const>());
        assert(second->is<LocalSet>());
        return std::make_shared<LocalSet_I32Const_LocalSet>(std::move(first), std::move(second));
    }

private:
    Operation local_set_i32_const_;
    Operation local_set_;

    void do_impl(Instance& instance, const std::array<Value, 1>& in) override {
        LocalSet::impl(local_set_->as<LocalSet>(), instance, LocalSet_I32Const::impl(local_set_i32_const_->as<LocalSet_I32Const>(), instance, in[0]));
    }
};

/************************/
/* Numeric Instructions */
/************************/

class I32Const_LocalSet : public TaggedOperation<I32Const_LocalSet> {
public:
    I32Const_LocalSet(Operation&& i32_const, Operation&& local_set)
        : i32_const_(std::move(i32_const)), local_set_(std::move(local_set)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<I32Const>());
        assert(second->is<LocalSet>());
        return std::make_shared<I32Const_LocalSet>(std::move(first), std::move(second));
    }

    Continuation action(Instance& instance) override {
        impl(*this, instance);
        return next_.get();
    }

private:
    Operation i32_const_;
    Operation local_set_;

    inline static void impl(I32Const_LocalSet& i32_const_local_set,
                            Instance& instance) {
        return LocalSet::impl(
            i32_const_local_set.local_set_->as<LocalSet>(), instance,
            I32Const::impl(i32_const_local_set.i32_const_->as<I32Const>(), instance));
    }

    friend class I32Const_LocalSet_LocalGet;
};

class I32Const_LocalSet_LocalGet : public GenericProducer<1> {
public:
    I32Const_LocalSet_LocalGet(Operation&& i32_const_local_set,
                               Operation&& local_get)
        : i32_const_local_set_(std::move(i32_const_local_set)), local_get_(std::move(local_get)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<I32Const_LocalSet>());
        assert(second->is<LocalGet>());
        return std::make_shared<I32Const_LocalSet_LocalGet>(std::move(first),
                                                            std::move(second));
    }

private:
    Operation i32_const_local_set_;
    Operation local_get_;

    std::array<Value, 1> do_impl(Instance& instance) override {
        I32Const_LocalSet::impl(i32_const_local_set_->as<I32Const_LocalSet>(), instance);
        return {LocalGet::impl(local_get_->as<LocalGet>(), instance)};
    }

    friend class LocalSet_LocalGet_LocalGet;
};

class I32Const_I32Const : public TaggedOperation<I32Const_I32Const> {
public:
    I32Const_I32Const(Operation&& i32_const1, Operation&& i32_const2)
        : i32_const1_(std::move(i32_const1)), i32_const2_(std::move(i32_const2)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<I32Const>());
        assert(second->is<I32Const>());
        return std::make_shared<I32Const_I32Const>(std::move(first), std::move(second));
    }

    Continuation action(Instance& instance) override {
        auto out = impl(*this, instance);
        instance.getActiveContext().getStack().push(out);
        return next_.get();
    }

private:
    Operation i32_const1_;
    Operation i32_const2_;

    inline static std::array<Value, 2>
    impl(I32Const_I32Const& i32_const_i32_const_, Instance& instance) {
        return {I32Const::impl(i32_const_i32_const_.i32_const1_->as<I32Const>(), instance),
                I32Const::impl(i32_const_i32_const_.i32_const2_->as<I32Const>(), instance)};
    }

    friend class I32Const_I32Const_I32Const;
};

class I32Const_I32Const_I32Const : public GenericProducer<3> {
public:
    I32Const_I32Const_I32Const(Operation&& i32_const_i32_const, Operation&& i32_const)
        : i32_const_i32_const_(std::move(i32_const_i32_const)), i32_const_(std::move(i32_const)) {}

    static Operation create(Operation&& first, Operation&& second) {
        assert(first->is<I32Const_I32Const>());
        assert(second->is<I32Const>());
        return std::make_shared<I32Const_I32Const_I32Const>(std::move(first), std::move(second));
    }

private:
    Operation i32_const_i32_const_;
    Operation i32_const_;

    std::array<Value, 3> do_impl(Instance& instance) override {
        auto first = I32Const_I32Const::impl(i32_const_i32_const_->as<I32Const_I32Const>(), instance);
        auto second = I32Const::impl(i32_const_->as<I32Const>(), instance);

        return { first[0], first[1], second };
    }
};

static size_t combineId(OperationBase::TypeId first,
                        OperationBase::TypeId second) {
    size_t seed = 0;
    hash_combine(seed, first);
    hash_combine(seed, second);
    return seed;
}

static const std::unordered_map<size_t, Operation (*)(Operation&&, Operation&&)>
    recipes = {
        // generic
        {combineId(GenericProducer<1>::static_type(), LocalGet::static_type()),
         GenericProducer_LocalGet<1>::create},
        // non generic
        {combineId(LocalGet::static_type(), LocalGet::static_type()),
         LocalGet_LocalGet::create},
         {combineId(LocalGet::static_type(), I32Const::static_type()),
         LocalGet_I32Const::create},
        {combineId(LocalSet::static_type(), LocalGet::static_type()),
         LocalSet_LocalGet::create},
        {combineId(LocalSet_LocalGet::static_type(), LocalGet::static_type()),
         LocalSet_LocalGet_LocalGet::create},
         {combineId(LocalSet::static_type(), I32Const::static_type()),
         LocalSet_I32Const::create},
         {combineId(LocalSet_I32Const::static_type(), LocalSet::static_type()),
         LocalSet_I32Const_LocalSet::create},
        {combineId(I32Const::static_type(), LocalSet::static_type()),
         I32Const_LocalSet::create},
        {combineId(I32Const_LocalSet::static_type(), LocalGet::static_type()),
         I32Const_LocalSet_LocalGet::create},
        {combineId(I32Const::static_type(), I32Const::static_type()),
         I32Const_I32Const::create},
        {combineId(I32Const_I32Const::static_type(), I32Const::static_type()),
         I32Const_I32Const_I32Const::create},
};

static std::unordered_map<size_t, std::string> names;
static std::map<size_t, size_t> misses;
static size_t mergesPerformed = 0;
static size_t mergeAttempts = 0;

bool canMerge(const Operation& first, const Operation& second) {
    if (first->is<Label>() || second->is<Label>())
        return false;

    mergeAttempts++;

    static const std::unordered_set<OperationBase::TypeId> BRANCHING_OPERATIONS = {
        BranchIf::static_type(),
        Call::static_type(),
        I32Store::static_type(),
    };

    if (BRANCHING_OPERATIONS.contains(first->type()))
        return false;

    if (first->is<Nop>())
        return true;

    size_t combinedId = combineId(first->type(), second->type());
    if (!recipes.contains(combinedId)) {
        if (!names.contains(combinedId)) {
            names[combinedId] =
                fmt::format("{}, {}", first->getName(), second->getName());
            misses[combinedId] = 0;
        } else {
            misses[combinedId]++;
        }

        return false;
    }

    mergesPerformed++;
    return true;
}

Operation merge(Operation&& first, Operation&& second) {
    mergesPerformed++;

    if (first->is<Nop>())
        return second;

    size_t combinedId = combineId(first->type(), second->type());

    auto it = recipes.find(combinedId);
    if (it == recipes.end()) {
        std::exit(-1);
    }

    return it->second(std::move(first), std::move(second));
}

void printStats() {
    std::vector<std::pair<std::string, size_t>> stats;
    stats.reserve(misses.size());

    for (const auto& [id, count] : misses) {
        auto it = names.find(id);
        if (it != names.end() && count > 0) {
            stats.emplace_back(it->second, count);
        }
    }

    std::sort(stats.begin(), stats.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });

    fmt::println("=== Optimization Stats ===");
    fmt::println("merge attempts: {}", mergeAttempts);
    fmt::println("merge success rate: {}%\n",
               (mergesPerformed * 100.0f) / mergeAttempts);

    size_t limit = std::min<size_t>(10, stats.size());
    fmt::println("=== Top {} Optimization Misses ===", limit);
    for (size_t i = 0; i < limit; i++) {
        fmt::println("{:>2.4f}% : {}", (100.0f * stats[i].second) / mergeAttempts, stats[i].first);
    }
}

} // namespace runtime