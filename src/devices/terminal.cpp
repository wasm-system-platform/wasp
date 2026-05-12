#include <iostream>
#include <utility>

#include <fcntl.h>
#include <unistd.h>

#include "devices/terminal.hpp"

namespace dev {

Terminal::Terminal() : DeviceBase(VENDOR_ID, DEVICE_ID) {
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);
}

bool Terminal::tick() {
    char c;
    if (::read(STDIN_FILENO, &c, 1) == 1) {
        std::lock_guard<std::mutex> guard(guard_);

        input_.push_back(c);
        return true;
    }

    return false;
}

void Terminal::io(Instance& instance, int32_t cmd, std::span<uint8_t> buffer) {
    switch (cmd) {
    case std::to_underlying(Command::read):
        read(buffer);
        break;
    case std::to_underlying(Command::write):
        write(buffer);
        break;
    default:
        fmt::println("unknown cmd: {}", cmd);
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
    }
}

void Terminal::read(std::span<uint8_t> buffer) {
    struct ReadCommand {
        Result result;
        uint32_t bytes_read;
        uint32_t buffer_size;
        char buffer[];
    } __attribute__((packed));

    if (buffer.size() < sizeof(ReadCommand)) {
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
        return;
    }

    ReadCommand* cmd = reinterpret_cast<ReadCommand*>(buffer.data());
    if (buffer.size() < sizeof(ReadCommand) + cmd->buffer_size) {
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

    cmd->bytes_read = i;
    cmd->result = Result::success;
}

void Terminal::write(std::span<uint8_t> buffer) {
    struct WriteCommand {
        Result result;
        uint32_t bytes_written;
        uint32_t buffer_size;
        char buffer[];
    } __attribute__((packed));

    if (buffer.size() < sizeof(WriteCommand)) {
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
        return;
    }

    WriteCommand* cmd = reinterpret_cast<WriteCommand*>(buffer.data());
    if (buffer.size() < sizeof(WriteCommand) + cmd->buffer_size) {
        cmd->result = Result::invalid_arguments;
        return;
    }

    std::lock_guard<std::mutex> guard(guard_);

    for (uint32_t i = 0; i < cmd->buffer_size; ++i) {
        std::cout << cmd->buffer[i];
        std::cout.flush();
    }

    cmd->bytes_written = cmd->buffer_size;
    cmd->result = Result::success;
}

} // namespace dev
