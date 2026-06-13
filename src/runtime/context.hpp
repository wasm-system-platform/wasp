#pragma once

#include <cassert>
#include <cstdint>

#include "runtime/operations.hpp"

namespace runtime {

// using Value = std::variant<int32_t, int64_t, float, double>;

class Stack {
public:
    size_t size() const { return top; }

    Value& peek() { return data_[top - 1]; }

    inline Value pop() { return data_[--top]; }

    template <typename Tuple> inline Tuple pop() {
        top -= std::tuple_size_v<Tuple>;

        return [&]<size_t... Is>(std::index_sequence<Is...>) {
            return Tuple{static_cast<std::tuple_element_t<Is, Tuple>>(
                data_[top + Is])...};
        }(std::make_index_sequence<std::tuple_size_v<Tuple>>{});
    }

    inline void push(Value value) { data_[top++] = value; }

    template <typename... Values>
    inline void push(const std::tuple<Values...>& in) {
        std::apply(
            [this](const Values&... values) {
                ((data_[top++] = static_cast<Value>(values)), ...);
            },
            in);
    }

    inline void resize(size_t new_size) { top = new_size; }
    const Value* data() { return data_.data(); }

    void clear() { top = 0; }

private:
    size_t top = 0;
    std::array<Value, UINT16_MAX> data_;
};

class Locals {
public:
    void pushLocals(const std::vector<Value>& locals) {
        size_t frame_size = locals.size();

        if (stack_pointer_ + frame_size > values_.size()) {
            values_.resize(stack_pointer_ + frame_size);
        }

        std::memcpy(values_.data() + stack_pointer_, locals.data(),
                    frame_size * sizeof(Value));

        frames_.push_back(frame_pointer_);
        frame_pointer_ = stack_pointer_;
        stack_pointer_ += frame_size;
    }

    void popLocals() {
        stack_pointer_ = frame_pointer_;
        frame_pointer_ = frames_.back();
        frames_.pop_back();
    }

    Value getLocal(uint32_t idx) const { return values_[frame_pointer_ + idx]; }

    void setLocal(uint32_t idx, Value value) {
        values_[frame_pointer_ + idx] = value;
    }

    size_t getStackPointer() const { return stack_pointer_; }
    size_t getFramePointer() const { return frame_pointer_; }
    const std::vector<Value>& getValues() const { return values_; }
    const std::vector<size_t>& getFrames() const { return frames_; }

    void setFramePointer(size_t frame_pointer) {
        frame_pointer_ = frame_pointer;
    }
    void setStackPointer(size_t stack_pointer) {
        stack_pointer_ = stack_pointer;
    }
    void setValues(std::vector<Value>&& values) { values_ = std::move(values); }
    void setFrames(std::vector<size_t>&& frames) {
        frames_ = std::move(frames);
    }

private:
    size_t stack_pointer_ = 0;
    size_t frame_pointer_ = 0;
    std::vector<Value> values_;
    std::vector<size_t> frames_;
};

class Epilogues {
public:
    void push(const Operation& epilogue) { data_[top_++] = epilogue; }
    const Operation& pop() { return data_[--top_]; }
    const Operation& peek() { return data_[top_ - 1]; }
    void swap(const Operation& epilogue) { data_[top_ - 1] = epilogue; }
    void shrink(size_t new_size) { top_ = new_size; }
    size_t size() { return top_; }
    const Operation* data() const { return data_.data(); }
    void clear() { top_ = 0; }

private:
    size_t top_ = 0;
    std::array<Operation, UINT16_MAX> data_;
};

class OperationBase;
using Continuation = OperationBase*;

class Context {
public:
    enum class RunState { rdy, running, waiting, in_syscall, suspended };

    Context(size_t id) : id_(id) {}
    Context(const Context& other, size_t id)
        : id_(id), locals_(other.locals_), stack_(other.stack_),
          epilogues_(other.epilogues_), run_state_(other.run_state_) {}

    inline size_t getId() const { return id_; }

    size_t size() const { return stack_.size(); }
    inline void push(Value value) { stack_.push(value); }
    inline void pushI32(int32_t value) { stack_.push(value); }
    inline void pushI64(int64_t value) { stack_.push(value); }

    inline Stack& getStack() { return stack_; }
    inline Value pop() { return stack_.pop(); }
    inline void drop() { stack_.pop(); }

    inline Locals& getLocals() { return locals_; }
    void pushLocals(const std::vector<Value>& locals) {
        return locals_.pushLocals(locals);
    }
    inline void popLocals() { locals_.popLocals(); }
    inline Value getLocal(uint32_t idx) { return locals_.getLocal(idx); }
    inline void setLocal(uint32_t idx, Value value) {
        locals_.setLocal(idx, value);
    }

    void setRunState(RunState state) { run_state_ = state; }
    RunState getRunState() const { return run_state_; }

    Epilogues& getEpilogues() { return epilogues_; }

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

    Locals locals_;
    Stack stack_;
    Epilogues epilogues_;

    RunState run_state_;

    uint32_t wait_addr_;
    int64_t timeout_;
};

} // namespace runtime
