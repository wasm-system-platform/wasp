#pragma once

#include <functional>
#include <variant>

#include "devices/device.hpp"
#include "grammar/module.hpp"
#include "runtime/context.hpp"
#include "runtime/function.hpp"
#include "runtime/operations.hpp"

namespace runtime {

using Params = std::vector<Value>;
using Result = std::optional<Value>;

/*
class FunctionInstances {
public:
    static Expected<FunctionInstances>
    create(const grammar::Module& module,
           const std::vector<ExternalFunction>& imports);

    inline Continuation invoke(size_t idx, Context& stack, Store& store) {
        // call external function
        if (idx < externals_.size()) {
            const ExternalFunction& external = externals_[idx];

            Params params(external.getNumParams());
            for (auto it = params.rbegin(); it != params.rend(); ++it) {
                *it = stack.pop();
            }

            Result result = external.exec(params);
            if (result.has_value())
                stack.push(*result);

            return nullptr;
        }

        // call internal function
        internals_[idx - externals_.size()].invoke();
    }

private:
    std::vector<ExternalFunction> externals_;
    std::vector<InternalFunction> internals_;

    FunctionInstances(std::vector<ExternalFunction>&& externals,
                      std::vector<InternalFunction>&& internals);
}; */

/********************/
/* Memory Instances */
/********************/

class MemoryInstances {
public:
    static Expected<MemoryInstances> create(const grammar::Module& module);

    inline const std::vector<uint8_t>& getHeap() const { return heap_; }
    inline std::vector<uint8_t>& getHeap() { return heap_; }

private:
    std::vector<uint8_t> heap_;
    uint32_t max_;
    bool shared_;

    MemoryInstances(std::vector<uint8_t>&& heap, uint32_t max, bool shared)
        : heap_(std::move(heap)), max_(max), shared_(shared) {}
};

/********************/
/* Global Instances */
/********************/

class GlobalInstances {
public:
    static Expected<GlobalInstances> create(const grammar::Module& module);

    inline Value getGlobal(uint32_t idx) const { return globals_[idx]; }
    inline Value setGlobal(uint32_t idx, Value value) {
        return globals_[idx] = value;
    }

private:
    std::vector<Value> globals_;

    GlobalInstances(std::vector<Value>&& globals)
        : globals_(std::move(globals)) {}
};

/******************/
/* Data Instances */
/******************/

class DataInstances {
public:
    static Expected<DataInstances> create(const grammar::Module& module);

    inline std::vector<std::vector<uint8_t>>& getSegments() {
        return segments_;
    }

private:
    std::vector<std::vector<uint8_t>> segments_;

    DataInstances(std::vector<std::vector<uint8_t>>&& segments)
        : segments_(std::move(segments)) {}
};

/*********/
/* Store */
/*********/

class GlobalState {
public:
    static GlobalState empty();
    static Expected<GlobalState>
    create(const grammar::Module& module,
           const std::unordered_map<std::string, Function>& imports);

    inline const std::vector<uint8_t>& getMemory() const {
        return mems_.getHeap();
    }
    inline std::vector<uint8_t>& getMemory() { return mems_.getHeap(); }

    inline std::vector<std::vector<uint8_t>>& getData() {
        return datas_.getSegments();
    }

    inline void dropSegment(uint32_t segment_idx) {
        datas_.getSegments()[segment_idx].clear();
    }

    inline Value getGlobal(uint32_t idx) const {
        return globals_.getGlobal(idx);
    }
    inline void setGlobal(uint32_t idx, Value val) {
        globals_.setGlobal(idx, val);
    }

    inline Function& getFunction(uint32_t idx) { return funcs_[idx]; }
    inline Function& getFunctionIndirect(uint32_t idx) {
        return funcs_[indirect_funcs_[idx]];
    }

    void enableInterrupts() { interrupts_enabled_ = true; }
    bool disableInterrupts() {
        bool old = interrupts_enabled_;
        interrupts_enabled_ = false;
        return old;
    }
    Continuation tick(Instance& instance,
                      const std::unordered_map<uint32_t, Device>& devices) {
        Operation interrupt;

        for (const auto& [port, device] : devices) {
            interrupt = device->tick(instance, port, interrupt);
        }

        return interrupts_enabled_ ? interrupt.get() : nullptr;
    }

private:
    std::vector<Function> funcs_;
    std::vector<uint32_t> indirect_funcs_;

    MemoryInstances mems_;
    GlobalInstances globals_;
    DataInstances datas_;

    bool interrupts_enabled_ = false;

    GlobalState(std::vector<Function>&& funcs,
                std::vector<uint32_t>&& indirect_funcs,
                const MemoryInstances& mems, const GlobalInstances& globals,
                const DataInstances& datas)
        : funcs_(std::move(funcs)), indirect_funcs_(std::move(indirect_funcs)),
          mems_(mems), globals_(globals), datas_(datas) {}

    static Expected<std::vector<Function>>
    createFunctions(const grammar::Module& module,
                    const std::unordered_map<std::string, Function>& imports);

    static Expected<std::vector<uint32_t>>
    createIndirectFunctions(const grammar::Module& module);

    static Expected<int32_t>
    evaluateExpressionI32(const grammar::Expression& expression);
};

} // namespace runtime
