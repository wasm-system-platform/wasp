#include <fcntl.h>
#include <unistd.h>

#include "devices/keyboard.hpp"

Keyboard::Keyboard() {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

bool Keyboard::tick() {
    std::lock_guard<std::mutex> guard(guard_);
    char c;
    if (read(STDIN_FILENO, &c, 1) == 1) {
        input_.push_back(c);
        return true;
    }

    return false;
}

Errno Keyboard::io(int32_t cmd, std::span<uint8_t>& buffer) {
    switch (cmd) {
        case 1: return getInput(buffer);
        default: return Errno::INVALID_ARGUMENT;
    }
}

Errno Keyboard::getInput(std::span<uint8_t>& buffer) {
    if (buffer.size() < 1)
        return Errno::BUFFER_TO_SMALL;

    std::lock_guard<std::mutex> guard(guard_);
    if (input_.empty())
        return Errno::NO_DATA;


    buffer[0] = input_.front();
    input_.pop_front();
    return Errno::SUCCESS;
}
