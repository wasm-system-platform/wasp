#include <map>

#include "runtime/instance.hpp"
#include "runtime/operations_impl.hpp"
#include "runtime/optimization.hpp"
#include "util/hash.hpp"

namespace runtime {

/**********************/
/* Generic Operations */
/**********************/

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
    GenericProducer_LocalGet(GenericProducer<num_out>& producer,
                             LocalGet& local_get)
        : producer_(producer.shared_from_this()), local_get_(local_get) {}

    static Operation create(const Operation& first, const Operation& second) {
        assert(first->is<GenericProducer<num_out>>());
        assert(second->is<LocalGet>());

        GenericProducer<num_out>& producer =
            first->as<GenericProducer<num_out>>();
        LocalGet& local_get = second->as<LocalGet>();

        return std::make_shared<GenericProducer_LocalGet>(producer, local_get);
    }

private:
    Operation producer_;
    LocalGet local_get_;

    std::array<Value, num_out + 1> do_impl(Instance& instance) override {
        std::array<Value, num_out + 1> out;

        auto produced =
            producer_->as<GenericProducer<num_out>>().do_impl(instance);
        std::copy(produced.begin(), produced.end(), out.begin());

        out[num_out] = LocalGet::impl(local_get_, instance);

        return out;
    }
};

/*************************/
/* Variable Instructions */
/*************************/

class LocalGet_LocalGet : public TaggedOperation<LocalGet_LocalGet> {
public:
    LocalGet_LocalGet(LocalGet& local_get1, LocalGet& local_get2)
        : local_get1_(local_get1), local_get2_(local_get2) {}

    static Operation create(const Operation& first, const Operation& second) {
        assert(first->is<LocalGet>());
        assert(second->is<LocalGet>());

        LocalGet& local_get1 = first->as<LocalGet>();
        LocalGet& local_get2 = second->as<LocalGet>();

        return std::make_shared<LocalGet_LocalGet>(local_get1, local_get2);
    }

    Continuation action(Instance& instance) override {
        auto [local1, local2] = impl(*this, instance);
        Context& context = instance.getActiveContext();
        context.push(local1);
        context.push(local2);
        return next_.get();
    }

private:
    LocalGet local_get1_;
    LocalGet local_get2_;

    inline static std::array<Value, 2>
    impl(LocalGet_LocalGet& local_get_local_get_, Instance& instance) {
        return {LocalGet::impl(local_get_local_get_.local_get1_, instance),
                LocalGet::impl(local_get_local_get_.local_get2_, instance)};
    }
};

class LocalSet_LocalGet : public TaggedOperation<LocalSet_LocalGet> {
public:
    LocalSet_LocalGet(LocalSet& local_set, LocalGet& local_get)
        : local_set_(local_set), local_get_(local_get) {}

    static Operation create(const Operation& first, const Operation& second) {
        assert(first->is<LocalSet>());
        assert(second->is<LocalGet>());

        LocalSet& local_set = first->as<LocalSet>();
        LocalGet& local_get = second->as<LocalGet>();

        return std::make_shared<LocalSet_LocalGet>(local_set, local_get);
    }

    Continuation action(Instance& instance) override {
        Stack& stack = instance.getActiveContext().getStack();
        Value local_in = stack.pop();
        Value local_out = impl(*this, instance, local_in);
        stack.push(local_out);
        return next_.get();
    }

private:
    LocalSet local_set_;
    LocalGet local_get_;

    inline static Value impl(LocalSet_LocalGet& local_get_local_get_,
                             Instance& instance, Value local_in) {
        LocalSet::impl(local_get_local_get_.local_set_, instance, local_in);
        return LocalGet::impl(local_get_local_get_.local_get_, instance);
    }

    friend class LocalSet_LocalGet_LocalGet;
};

class LocalSet_LocalGet_LocalGet : public GenericPipe<1, 2> {
public:
    LocalSet_LocalGet_LocalGet(LocalSet_LocalGet& local_set_local_get,
                               LocalGet& local_get)
        : local_set_local_get_(local_set_local_get), local_get_(local_get) {}

    static Operation create(const Operation& first, const Operation& second) {
        assert(first->is<LocalSet_LocalGet>());
        assert(second->is<LocalGet>());

        LocalSet_LocalGet& local_set_local_get = first->as<LocalSet_LocalGet>();
        LocalGet& local_get = second->as<LocalGet>();

        return std::make_shared<LocalSet_LocalGet_LocalGet>(local_set_local_get,
                                                            local_get);
    }

private:
    LocalSet_LocalGet local_set_local_get_;
    LocalGet local_get_;

    std::array<Value, 2> do_impl(Instance& instance,
                                 const std::array<Value, 1>& in) override {
        return {LocalSet_LocalGet::impl(local_set_local_get_, instance, in[0]),
                LocalGet::impl(local_get_, instance)};
    }
};

/************************/
/* Numeric Instructions */
/************************/

class I32Const_LocalSet : public TaggedOperation<I32Const_LocalSet> {
public:
    I32Const_LocalSet(I32Const& i32_const, LocalSet& local_set)
        : i32_const_(i32_const), local_set_(local_set) {}

    static Operation create(const Operation& first, const Operation& second) {
        assert(first->is<I32Const>());
        assert(second->is<LocalSet>());

        I32Const& i32_const = first->as<I32Const>();
        LocalSet& local_set = second->as<LocalSet>();

        return std::make_shared<I32Const_LocalSet>(i32_const, local_set);
    }

    Continuation action(Instance& instance) override {
        impl(*this, instance);
        return next_.get();
    }

private:
    I32Const i32_const_;
    LocalSet local_set_;

    inline static void impl(I32Const_LocalSet& i32_const_local_set,
                            Instance& instance) {
        return LocalSet::impl(
            i32_const_local_set.local_set_, instance,
            I32Const::impl(i32_const_local_set.i32_const_, instance));
    }

    friend class I32Const_LocalSet_LocalGet;
};

class I32Const_LocalSet_LocalGet : public GenericProducer<1> {
public:
    I32Const_LocalSet_LocalGet(I32Const_LocalSet& i32_const_local_set,
                               LocalGet& local_get)
        : i32_const_local_set_(i32_const_local_set), local_get_(local_get) {}

    static Operation create(const Operation& first, const Operation& second) {
        assert(first->is<I32Const_LocalSet>());
        assert(second->is<LocalGet>());

        I32Const_LocalSet& i32_const_local_set = first->as<I32Const_LocalSet>();
        LocalGet& local_get = second->as<LocalGet>();

        return std::make_shared<I32Const_LocalSet_LocalGet>(i32_const_local_set,
                                                            local_get);
    }

private:
    I32Const_LocalSet i32_const_local_set_;
    LocalGet local_get_;

    std::array<Value, 1> do_impl(Instance& instance) override {
        I32Const_LocalSet::impl(i32_const_local_set_, instance);
        return {LocalGet::impl(local_get_, instance)};
    }

    friend class LocalSet_LocalGet_LocalGet;
};

static size_t combineId(OperationBase::TypeId first,
                        OperationBase::TypeId second) {
    size_t seed = 0;
    hash_combine(seed, first);
    hash_combine(seed, second);
    return seed;
}

static const std::unordered_map<size_t, Operation (*)(const Operation&,
                                                      const Operation&)>
    recipes = {
        // generic
        {combineId(GenericProducer<1>::static_type(), LocalGet::static_type()),
         GenericProducer_LocalGet<1>::create},
        // non generic
        {combineId(LocalGet::static_type(), LocalGet::static_type()),
         LocalGet_LocalGet::create},
        {combineId(LocalSet::static_type(), LocalGet::static_type()),
         LocalSet_LocalGet::create},
        {combineId(LocalSet_LocalGet::static_type(), LocalGet::static_type()),
         LocalSet_LocalGet_LocalGet::create},
        {combineId(I32Const::static_type(), LocalSet::static_type()),
         I32Const_LocalSet::create},
        {combineId(I32Const_LocalSet::static_type(), LocalGet::static_type()),
         I32Const_LocalSet_LocalGet::create},
};

static std::unordered_map<size_t, std::string> names;
static std::map<size_t, size_t> misses;
static size_t mergesPerformed = 0;
static size_t mergeAttempts = 0;

bool canMerge(const Operation& first, const Operation& second) {
    mergeAttempts++;

    if (first->is<Nop>())
        return true;

    if (second->is<Label>())
        return false;

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

Operation merge(const Operation& first, const Operation& second) {
    mergesPerformed++;

    if (first->is<Nop>())
        return second;

    size_t combinedId = combineId(first->type(), second->type());

    auto it = recipes.find(combinedId);
    if (it == recipes.end()) {
        std::exit(-1);
    }

    return it->second(first, second);
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

    fmt::print("=== Optimization Stats ===\n");
    fmt::print("merge success rate: {}%\n\n",
               (mergesPerformed * 100.0f) / mergeAttempts);

    fmt::print("=== Top 10 Optimization Misses ===\n");

    size_t limit = std::min<size_t>(10, stats.size());
    for (size_t i = 0; i < limit; i++) {
        fmt::print("{:>8} : {}\n", stats[i].second, stats[i].first);
    }
}

} // namespace runtime