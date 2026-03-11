#include "runtime/optimization.hpp"
#include "util/hash.hpp"
#include <map>

namespace runtime {

static size_t combineId(OperationBase::TypeId first, OperationBase::TypeId second) {
    size_t seed = 0;
    hash_combine(seed, first);
    hash_combine(seed, second);
    return seed;
}

static const std::unordered_map<size_t, Operation (*)(const Operation&, const Operation&)> recipes = {
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
            names[combinedId] = fmt::format("{}, {}", first->getName(), second->getName());
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
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });

    fmt::print("=== Optimization Stats ===\n");
    fmt::print("merge success rate: {}%\n\n", (mergesPerformed*100.0f) / mergeAttempts);

    fmt::print("=== Top 10 Optimization Misses ===\n");

    size_t limit = std::min<size_t>(10, stats.size());
    for (size_t i = 0; i < limit; i++) {
        fmt::print("{:>8} : {}\n", stats[i].second, stats[i].first);
    }
}

}