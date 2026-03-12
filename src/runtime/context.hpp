#pragma once

#include <cassert>
#include <cstdint>
#include <stack>
#include <variant>

#include "runtime/operations.hpp"

namespace runtime {

// using Value = std::variant<int32_t, int64_t, float, double>;

class Stack {
public:
    size_t size() const { return top; }

    Value& peek() { return data_[top - 1]; }

    inline Value pop() { return data_[--top]; }

    template <size_t num_values>
    inline void pop(std::array<Value, num_values>& out) {
        top -= num_values;
        std::memcpy(out.data(), &data_[top], sizeof(Value) * num_values);
    }

    inline void push(Value value) { data_[top++] = value; }

    template <size_t num_values>
    inline void push(const std::array<Value, num_values>& in) {
        std::memcpy(&data_[top], in.data(), sizeof(Value) * num_values);
        top += num_values;
    }

    inline void resize(size_t new_size) { top = new_size; }
    const Value* data() { return data_.data(); }

private:
    size_t top = 0;
    std::array<Value, UINT16_MAX> data_;
};

class Locals {
public:
    void pushLocals(const std::vector<Value>& locals) {
        size_t frame_size = locals.size();

        if (stack_pointer_ + frame_size > stack_.size()) {
            stack_.resize(stack_pointer_ + frame_size);
        }

        std::memcpy(stack_.data() + stack_pointer_, locals.data(),
                    frame_size * sizeof(Value));

        frame_pointers_.push_back(frame_pointer_);
        frame_pointer_ = stack_pointer_;
        stack_pointer_ += frame_size;
    }

    void popLocals() {
        stack_pointer_ = frame_pointer_;
        frame_pointer_ = frame_pointers_.back();
        frame_pointers_.pop_back();
    }

    Value getLocal(uint32_t idx) const { return stack_[frame_pointer_ + idx]; }

    void setLocal(uint32_t idx, Value value) {
        stack_[frame_pointer_ + idx] = value;
    }

    size_t getFramePointersSize() const { return frame_pointers_.size(); }

private:
    size_t stack_pointer_ = 0;
    size_t frame_pointer_ = 0;
    std::vector<size_t> frame_pointers_;
    std::vector<Value> stack_;
};

class Epilogues {
public:
    void push(const Operation& epilogue) { data_[top++] = epilogue; }
    const Operation& pop() { return data_[--top]; }
    const Operation& peek() { return data_[top - 1]; }
    void swap(const Operation& epilogue) { data_[top - 1] = epilogue; }
    void shrink(size_t new_size) { top = new_size; }
    size_t size() { return top; }
    const Operation* data() { return data_.data(); }

private:
    size_t top = 0;
    std::array<Operation, UINT16_MAX> data_;
};

class OperationBase;
using Continuation = OperationBase*;

class Context {
public:
    enum class RunState { rdy, running, waiting, in_syscall, suspended };

    Context(size_t id) : id_(id) {}
    Context(const Context& other, size_t id)
        : id_(id), locals_(std::make_unique<Locals>(*other.locals_)),
          stack_(std::make_unique<Stack>(*other.stack_)),
          epilogues_(std::make_unique<Epilogues>(*other.epilogues_)),
          run_state_(other.run_state_) {}

    inline size_t getId() const { return id_; }

    size_t size() const { return stack_->size(); }
    inline void push(Value value) { stack_->push(value); }
    inline void pushI32(int32_t value) { stack_->push(value); }
    inline void pushI64(int64_t value) { stack_->push(value); }

    inline Stack& getStack() { return *stack_; }
    inline Value pop() { return stack_->pop(); }
    inline void drop() { stack_->pop(); }

    inline Locals& getLocals() { return *locals_; }
    void pushLocals(const std::vector<Value>& locals) {
        return locals_->pushLocals(locals);
    }
    inline void popLocals() { locals_->popLocals(); }
    inline Value getLocal(uint32_t idx) { return locals_->getLocal(idx); }
    inline void setLocal(uint32_t idx, Value value) {
        locals_->setLocal(idx, value);
    }

    void setRunState(RunState state) { run_state_ = state; }
    RunState getRunState() const { return run_state_; }

    Epilogues& getEpilogues() { return *epilogues_; }

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

    std::unique_ptr<Locals> locals_ = std::make_unique<Locals>();
    std::unique_ptr<Stack> stack_ = std::make_unique<Stack>();
    std::unique_ptr<Epilogues> epilogues_ = std::make_unique<Epilogues>();

    RunState run_state_;

    uint32_t wait_addr_;
    int64_t timeout_;
};

} // namespace runtime
