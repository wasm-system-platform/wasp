#include <chrono>
#include <utility>

#include "devices/timer.hpp"

bool Timer::tick() {
    std::lock_guard<std::mutex> guard(guard_);

    if (!running_)
        return false;

    const auto now = std::chrono::steady_clock::now();
    const auto elapsed_since_last_tick =
        std::chrono::duration_cast<std::chrono::nanoseconds>(now - last_tick_);

    accumulated_ns_ += elapsed_since_last_tick.count();

    const bool interval_elapsed = accumulated_ns_ >= interval_ns_;
    const bool unlimited_fires = max_fires_ == UINT32_MAX;

    if (interval_elapsed) {
        accumulated_ns_ -= interval_ns_;

        if (!unlimited_fires) {
            fires_done_++;

            if (fires_done_ >= max_fires_)
                running_ = false;
        }
    }

    last_tick_ = now;
    return interval_elapsed;
}

void Timer::io(Instance& instance, int32_t cmd, std::span<uint8_t> buffer) {
    switch (cmd) {
    case std::to_underlying(Command::start):
        start(buffer);
        break;
    case std::to_underlying(Command::stop):
        stop(buffer);
        break;
    case std::to_underlying(Command::set):
        set(buffer);
        break;
    case std::to_underlying(Command::get_time):
        getTime(buffer);
        break;
    default:
        fmt::println("unknown cmd: {}", cmd);
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
    }
}

void Timer::start(std::span<uint8_t> buffer) {
    Result* result = reinterpret_cast<Result*>(buffer.data());

    std::lock_guard<std::mutex> guard(guard_);

    running_ = true;
    last_tick_ = std::chrono::steady_clock::now();

    *result = Result::success;
}

void Timer::stop(std::span<uint8_t> buffer) {
    Result* result = reinterpret_cast<Result*>(buffer.data());

    std::lock_guard<std::mutex> guard(guard_);

    running_ = false;

    *result = Result::success;
}

void Timer::set(std::span<uint8_t> buffer) {
    struct SetCommand {
        Result result;
        uint64_t timeout_ns;
        uint32_t repeat_count;
    } __attribute__((packed));

    if (buffer.size() < sizeof(SetCommand)) {
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
        return;
    }

    SetCommand* cmd = reinterpret_cast<SetCommand*>(buffer.data());

    std::lock_guard<std::mutex> guard(guard_);

    accumulated_ns_ = 0;
    interval_ns_ = cmd->timeout_ns & ~(1ULL << 63);
    max_fires_ = cmd->repeat_count;

    cmd->result = Result::success;
}

void Timer::getTime(std::span<uint8_t> buffer) {
    struct GetTimeCommand {
        Result result;
        uint64_t timestamp;
    } __attribute__((packed));

    if (buffer.size() < sizeof(GetTimeCommand)) {
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
        return;
    }

    GetTimeCommand* cmd = reinterpret_cast<GetTimeCommand*>(buffer.data());

    cmd->timestamp = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
    cmd->result = Result::success;
}
