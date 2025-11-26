#include <fcntl.h>

#include "devices/keyboard.hpp"
#include "runtime/instance.hpp"

Keyboard::Keyboard(uint32_t interrupt_handler_idx)
    : DeviceBase(interrupt_handler_idx) {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

runtime::Operation Keyboard::tick(runtime::Instance& instance, uint32_t port,
                                  runtime::Operation interrupt) {
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        intput_.push(c);
    }

    if (!intput_.empty()) {
        instance.getActiveContext().pushI32(port);
        handler_call->clearNext();
        handler_call->addNext(interrupt);
        return handler_call;
    }

    return interrupt;
}
