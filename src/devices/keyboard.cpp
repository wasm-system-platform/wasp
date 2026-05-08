#include <utility>

#include <fcntl.h>
#include <unistd.h>

#include "devices/keyboard.hpp"

Keyboard::Keyboard() : DeviceBase(VENDOR_ID, DEVICE_ID) {
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

void Keyboard::io(Instance& instance, int32_t cmd, std::span<uint8_t> buffer) {
    switch (cmd) {
    case std::to_underlying(Command::get_input):
        getInput(buffer);
        break;
    default:
        fmt::println("unknown cmd: {}", cmd);
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
    }
}

void Keyboard::getInput(std::span<uint8_t> buffer) {
    struct GetInputCommand {
        Result result;
        uint32_t chars_read;
        uint32_t buffer_size;
        char buffer[];
    } __attribute__((packed));

    if (buffer.size() < sizeof(GetInputCommand)) {
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
        return;
    }

    GetInputCommand* cmd = reinterpret_cast<GetInputCommand*>(buffer.data());
    if (buffer.size() < sizeof(GetInputCommand) + cmd->buffer_size) {
        cmd->result = Result::invalid_arguments;
        return;
    }

    std::lock_guard<std::mutex> guard(guard_);

    size_t to_read =
        std::min(static_cast<size_t>(cmd->buffer_size), input_.size());

    size_t i = 0;

    while (i < to_read && !input_.empty()) {
        cmd->buffer[i++] = input_.front();
        input_.pop_front();
    }

    cmd->chars_read = i;
    cmd->result = Result::success;
}
