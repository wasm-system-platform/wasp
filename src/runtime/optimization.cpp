#include <algorithm>
#include <map>

#include "runtime/instance.hpp"
#include "runtime/operations_impl.hpp"
#include "runtime/optimization.hpp"
#include "util/hash.hpp"

namespace runtime {

static size_t combineId(OperationBase::TypeId first,
                        OperationBase::TypeId second) {
    size_t seed = reinterpret_cast<size_t>(first);
    hash_combine(seed, second);
    return seed;
}

template <typename Op1, typename Op2, size_t num_in, size_t num_out>
class Sequence : public GenericOperation<Sequence<Op1, Op2, num_in, num_out>> {
public:
    using InType = Values<num_in>;
    using OutType = Values<num_out>;

    static std::pair<size_t, Operation (*)(Operation&, Operation&)> recipe() {
        return {combineId(decltype(first_)::static_type(),
                          decltype(second_)::static_type()),
                create};
    }

    static Operation create(Operation& first, Operation& second) {
        assert(first->is<decltype(first_)>());
        assert(second->is<decltype(second_)>());
        return std::make_shared<Sequence>(first, second);
    }

    template <typename... Args>
    static OutType impl(Sequence& seq, Instance& instance, Args... in);

    Sequence(Operation& first, Operation& second)
        : first_(first->as<decltype(first_)>()),
          second_(second->as<decltype(second_)>()) {}

    std::string getName() override {
        return fmt::format("{}_{}", first_.getName(), second_.getName());
    }

private:
    Op1 first_;
    Op2 second_;
};

template <typename Op1, typename Op2, size_t num_in, size_t num_out>
class Junction : public TaggedOperation<Junction<Op1, Op2, num_in, num_out>> {
public:
    using InType = Values<num_in>;
    using OutType = Values<num_out>;

    static std::pair<size_t, Operation (*)(Operation&, Operation&)> recipe() {
        return {combineId(decltype(first_)::static_type(),
                          decltype(second_)::static_type()),
                create};
    }

    static Operation create(Operation& first, Operation& second) {
        assert(first->is<decltype(first_)>());
        assert(second->is<decltype(second_)>());
        return std::make_shared<Junction>(first, second);
    }

    template <typename... Args>
    static Continuation impl(Junction& junc, Instance& instance, Args... in);

    Junction(Operation& first, Operation& second)
        : first_(first->as<decltype(first_)>()),
          second_(second->as<decltype(second_)>()) {}

    std::string getName() override {
        return fmt::format("{}_{}", first_.getName(), second_.getName());
    }

    Continuation action(Instance& instance) override {
        Stack& stack = instance.getActiveContext().getStack();

        if constexpr (std::is_same_v<InType, void>) {
            return impl(*this, instance);
        } else if constexpr (std::is_same_v<InType, Value>) {
            Value in = stack.pop();
            return impl(*this, instance, in);
        } else {
            auto in = stack.pop<InType>();
            return std::apply(
                [&](auto&&... args) {
                    return impl(*this, instance,
                                std::forward<decltype(args)>(args)...);
                },
                in);
        }
    }

private:
    Op1 first_;
    Op2 second_;
};

/* LocalGet_IfThen */

using LocalGet_IfThen = Junction<LocalGet, IfThen, 0, 0>;

template <>
template <typename... Args>
Continuation LocalGet_IfThen::impl(Junction& junc, Instance& instance,
                                   Args...) {
    return IfThen::impl(junc.second_, instance, junc.next_.get(),
                        LocalGet::impl(junc.first_, instance));
}

/* LocalGet_IfElse */

using LocalGet_IfElse = Junction<LocalGet, IfElse, 0, 0>;

template <>
template <typename... Args>
Continuation LocalGet_IfElse::impl(Junction& junc, Instance& instance,
                                   Args...) {
    return IfElse::impl(junc.second_, instance,
                        LocalGet::impl(junc.first_, instance));
}

/* LocalGet_LocalGet */

using LocalGet_LocalGet = Sequence<LocalGet, LocalGet, 0, 2>;

template <>
template <typename... Args>
inline LocalGet_LocalGet::OutType
LocalGet_LocalGet::impl(Sequence& seq, Instance& instance, Args...) {
    return {LocalGet::impl(seq.first_, instance),
            LocalGet::impl(seq.second_, instance)};
}

/* LocalGet_LocalGet_I32Const */

using LocalGet_LocalGet_I32Const = Sequence<LocalGet_LocalGet, I32Const, 0, 3>;

template <>
template <typename... Args>
inline LocalGet_LocalGet_I32Const::OutType
LocalGet_LocalGet_I32Const::impl(Sequence& seq, Instance& instance, Args...) {
    auto [v1, v2] = LocalGet_LocalGet::impl(seq.first_, instance);
    return {v1, v2, I32Const::impl(seq.second_, instance)};
}

/* LocalGet_LocalGet_I32Const_I32Sub */

using LocalGet_LocalGet_I32Const_I32Sub =
    Sequence<LocalGet_LocalGet_I32Const, I32Sub, 0, 2>;

template <>
template <typename... Args>
inline LocalGet_LocalGet_I32Const_I32Sub::OutType
LocalGet_LocalGet_I32Const_I32Sub::impl(Sequence& seq, Instance& instance,
                                        Args...) {
    auto [v1, v2, v3] = LocalGet_LocalGet_I32Const::impl(seq.first_, instance);
    return {v1, I32Sub::impl(seq.second_, instance, v2, v3)};
}

/* LocalGet_I32Const */

using LocalGet_I32Const = Sequence<LocalGet, I32Const, 0, 2>;

template <>
template <typename... Args>
inline LocalGet_I32Const::OutType
LocalGet_I32Const::impl(Sequence& seq, Instance& instance, Args...) {
    return {LocalGet::impl(seq.first_, instance),
            I32Const::impl(seq.second_, instance)};
}

/* LocalGet_I32Const_I32Add */

using LocalGet_I32Const_I32Add = Sequence<LocalGet_I32Const, I32Add, 0, 1>;

template <>
template <typename... Args>
inline LocalGet_I32Const_I32Add::OutType
LocalGet_I32Const_I32Add::impl(Sequence& seq, Instance& instance, Args...) {
    auto [v1, v2] = LocalGet_I32Const::impl(seq.first_, instance);
    return I32Add::impl(seq.second_, instance, v1, v2);
}

/* LocalSet_LocalGet */

using LocalSet_LocalGet = Sequence<LocalSet, LocalGet, 1, 1>;

template <>
template <typename... Args>
inline LocalSet_LocalGet::OutType
LocalSet_LocalGet::impl(Sequence& seq, Instance& instance, Args... in) {
    LocalSet::impl(seq.first_, instance, in...);
    return LocalGet::impl(seq.second_, instance);
}

/* LocalSet_LocalGet_I32Const */

using LocalSet_LocalGet_I32Const = Sequence<LocalSet_LocalGet, I32Const, 1, 2>;

template <>
template <typename... Args>
inline LocalSet_LocalGet_I32Const::OutType
LocalSet_LocalGet_I32Const::impl(Sequence& seq, Instance& instance,
                                 Args... in) {
    return {LocalSet_LocalGet::impl(seq.first_, instance, in...),
            I32Const::impl(seq.second_, instance)};
}

/* LocalSet_LocalGet_I32Const_I32Sub */

using LocalSet_LocalGet_I32Const_I32Sub =
    Sequence<LocalSet_LocalGet_I32Const, I32Sub, 1, 1>;

template <>
template <typename... Args>
inline LocalSet_LocalGet_I32Const_I32Sub::OutType
LocalSet_LocalGet_I32Const_I32Sub::impl(Sequence& seq, Instance& instance,
                                        Args... in) {
    auto [v1, v2] =
        LocalSet_LocalGet_I32Const::impl(seq.first_, instance, in...);
    return I32Sub::impl(seq.second_, instance, v1, v2);
}

/* LocalTee_BranchIf */

using LocalTee_BranchIf = Junction<LocalTee, BranchIf, 1, 0>;

template <>
template <typename... Args>
Continuation LocalTee_BranchIf::impl(Junction& junc, Instance& instance,
                                     Args... in) {
    return BranchIf::impl(junc.second_, instance, junc.next_.get(),
                          LocalTee::impl(junc.first_, instance, in...));
}

/* I32Const_I32Const */

using I32Const_I32Const = Sequence<I32Const, I32Const, 0, 2>;

template <>
template <typename... Args>
inline I32Const_I32Const::OutType
I32Const_I32Const::impl(Sequence& seq, Instance& instance, Args...) {
    return {I32Const::impl(seq.first_, instance),
            I32Const::impl(seq.second_, instance)};
}

static const std::unordered_map<size_t, Operation (*)(Operation&, Operation&)>
    recipes = {
        LocalGet_IfThen::recipe(),
        LocalGet_IfElse::recipe(),
        LocalGet_LocalGet::recipe(),
        LocalGet_LocalGet_I32Const::recipe(),
        LocalGet_LocalGet_I32Const_I32Sub::recipe(),
        LocalGet_I32Const::recipe(),
        LocalGet_I32Const_I32Add::recipe(),
        LocalSet_LocalGet::recipe(),
        LocalSet_LocalGet_I32Const::recipe(),
        LocalSet_LocalGet_I32Const_I32Sub::recipe(),
        LocalTee_BranchIf::recipe(),
        I32Const_I32Const::recipe(),
};

static std::unordered_map<size_t, std::string> names;
static std::map<size_t, size_t> misses;
static size_t mergesPerformed = 0;
static size_t mergeAttempts = 0;

bool canMerge(const Operation& first, const Operation& second) {
    mergeAttempts++;

    if (first->is<Label>() || second->is<Label>())
        return false;

    static const std::unordered_set<OperationBase::TypeId>
        BRANCHING_OPERATIONS = {
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
            misses[combinedId] = 1;
        } else {
            misses[combinedId]++;
        }

        return false;
    }

    return true;
}

Operation merge(Operation& first, Operation& second) {
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

    fmt::println("=== Optimization Stats ===");
    fmt::println("merge attempts: {}", mergeAttempts);
    fmt::println("merge success rate: {}%\n",
                 (static_cast<double>(mergesPerformed) * 100.0) /
                     static_cast<double>(mergeAttempts));

    size_t limit = std::min<size_t>(10, stats.size());
    fmt::println("=== Top {} Optimization Misses ===", limit);
    for (size_t i = 0; i < limit; i++) {
        fmt::println("{:>2.4f}% : {}",
                     (static_cast<double>(100.0) *
                      static_cast<double>(stats[i].second)) /
                         static_cast<double>(mergeAttempts),
                     stats[i].first);
    }
}

} // namespace runtime