#include "devices/timer.hpp"
#include "runtime/instance.hpp"

Timer::Timer(uint32_t interrupt_handler_idx)
    : DeviceBase(interrupt_handler_idx) {}

runtime::Operation Timer::tick(runtime::Instance& instance, uint32_t port,
                               runtime::Operation interrupt) {
    if (!t_.active_ || t_.remaining_ == 0)
        return interrupt;

    t_.remaining_ -= 1;
    if (t_.remaining_ == 0) {
        if (t_.interval_mode_)
            t_.remaining_ = t_.interval_;
        else
            t_.active_ = false;

        instance.getActiveContext().pushI32(port);
        handler_call->clearNext();
        handler_call->addNext(interrupt);
        return handler_call;
    }

    return interrupt;
}

void Timer::setTimout(uint32_t timeout) {
    t_.remaining_ = timeout;
    t_.active_ = true;
    t_.interval_mode_ = false;
}

void Timer::setInterval(uint32_t timeout) {
    t_.remaining_ = timeout;
    t_.interval_ = timeout;
    t_.active_ = true;
    t_.interval_mode_ = true;
}

void Timer::clear() { t_ = {}; }
