#pragma once

#include <cassert>
#include <cstdint>
#include <stack>
#include <variant>

#include "runtime/operations.hpp"

namespace runtime {

using Value = std::variant<int32_t, int64_t, float, double>;

class OperationBase;
using Continuation = OperationBase*;

class Context : public std::enable_shared_from_this<Context> {
public:
    enum class RunState { rdy, running, waiting };

    Context(size_t id) : id_(id) {}

    inline size_t getId() const { return id_; }

    size_t size() const { return values_.size(); }
    inline void push(Value value) { values_.push(value); }
    inline void pushI32(int32_t value) { values_.push(value); }
    inline void pushI64(int64_t value) { values_.push(value); }
    inline Value pop() {
        Value top = values_.top();
        values_.pop();
        return top;
    }
    inline int32_t popI32() {
        int32_t top = std::get<int32_t>(values_.top());
        values_.pop();
        return top;
    }
    inline int64_t popI64() {
        int64_t top = std::get<int64_t>(values_.top());
        values_.pop();
        return top;
    }
    inline double popF64() {
        double top = std::get<double>(values_.top());
        values_.pop();
        return top;
    }
    inline void drop() { values_.pop(); }

    inline void pushLocals(const std::vector<Value>& locals) {
        locals_.push(locals);
    }
    inline void popLocals() { locals_.pop(); }
    inline Value getLocal(uint32_t idx) { return locals_.top()[idx]; }
    inline void setLocal(uint32_t idx, Value val) { locals_.top()[idx] = val; }

    void setRunState(RunState state) { run_state_ = state; }
    RunState getRunState() const { return run_state_; }

    inline void pushReturn(Continuation ret) { ret_stack_.push(ret); }
    inline Continuation popReturn() {
        Continuation top = ret_stack_.top();
        ret_stack_.pop();
        return top;
    }

    // waiting
    inline void setTimeout(uint32_t addr, int64_t timout) {
        wait_addr_ = addr;
        timeout_ = timout;
    }
    inline uint32_t getWaitAddr() const { return wait_addr_; }
    inline int64_t getRemainingTime() {
        if (timeout_ < 0)
            return INT64_MAX;
        if (timeout_ > 0)
            timeout_ -= 1;
        return timeout_;
    }

private:
    size_t id_;

    std::stack<Value> values_;
    std::stack<std::vector<Value>> locals_;
    std::vector<Operation> control_stack_;
    std::stack<Continuation> unwind_stack_;
    std::stack<Continuation> ret_stack_;

    RunState run_state_;

    uint32_t wait_addr_;
    int64_t timeout_;
};

} // namespace runtime
