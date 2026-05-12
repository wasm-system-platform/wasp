#pragma once

#include <atomic>

#include "runtime/context.hpp"
#include "runtime/handler.hpp"
#include "runtime/operations.hpp"

namespace runtime {

constexpr uint32_t INVALID_PORT = -1;

class InterruptController {
public:
    InterruptController(uint32_t interrupt_handler_idx)
        : interrupt_handler_(
              std::make_shared<Interrupt>(*this, interrupt_handler_idx)) {}

    bool getInterruptsEnabled() { return interrupts_enabled_; }
    bool enableInterrupts() {
        bool old = interrupts_enabled_;
        interrupts_enabled_ = true;
        return old;
    }
    bool disableInterrupts() {
        bool old = interrupts_enabled_;
        interrupts_enabled_ = false;
        return old;
    }

    Continuation next(Context& ctxt) {
        uint32_t port = interrupts_->pop();
        if (port == INVALID_PORT)
            return nullptr;

        ctxt.pushI32(port);
        return interrupt_handler_.get();
    }

    void idle() { interrupts_->wait(); }

    void interrupt(uint32_t port) { interrupts_->push(port); }

private:
    class Buffer {
    public:
        void push(uint32_t port) {
            const uint32_t head = head_.load();
            const uint32_t next = (head + 1) % data_.size();

            while (next == tail_.load()) {
                tail_.wait(next);
            }

            data_[head] = port;
            head_.store(next);
            head_.notify_all();
        }

        uint32_t pop() {
            size_t tail = tail_.load();

            if (tail == head_.load())
                return INVALID_PORT;

            uint32_t port = data_[tail];
            tail_.store((tail + 1) % data_.size());
            tail_.notify_one();
            return port;
        }

        void wait() {
            size_t tail = tail_.load();

            if (tail == head_.load()) {
                head_.wait(tail);
            }
        }

    private:
        std::atomic<uint32_t> head_{0};
        std::atomic<uint32_t> tail_{0};
        std::array<uint32_t, UINT8_MAX> data_;
    };

    std::unique_ptr<Buffer> interrupts_ = std::make_unique<Buffer>();

    bool interrupts_enabled_ = false;
    Operation interrupt_handler_;
};

} // namespace runtime
