#pragma once

#include "fmt/base.h"
#include <cstdint>
#include <span>

class ByteCursor {
public:
    explicit ByteCursor(std::span<const std::uint8_t> bytes)
        : begin_(bytes.data()), cur_(bytes.data()), end_(bytes.data() + bytes.size()) {}

    bool eof() const {
        return cur_ == end_;
    }

    bool bad() const {
        return badbit_;
    }

    size_t offset() const {
        return static_cast<size_t>(cur_ - begin_);
    }

    uint8_t byte() {
        if (badbit_)
            return 0xFF;

        if (cur_ == end_) {
            badbit_ = true;
            return 0xFF;
        }

        return *cur_++;
    }

    uint8_t peeek() {
        if (badbit_)
            return 0xFF;

        if (cur_ == end_) {
            badbit_ = true;
            return 0xFF;
        }
        
        return *cur_;
    }

    uint16_t u16_le() {
        auto bytes = read(2);

        if (badbit_)
            return 0xFF'FF;

        return static_cast<uint16_t>(
             static_cast<uint16_t>(bytes[0])
           | static_cast<uint16_t>(bytes[1]) << 8);
    }

    uint32_t u32_le() {
        auto bytes = read(4);

        if (badbit_)
            return 0;

        return static_cast<uint32_t>(bytes[0])
             | static_cast<uint32_t>(bytes[1]) << 8
             | static_cast<uint32_t>(bytes[2]) << 16
             | static_cast<uint32_t>(bytes[3]) << 24;
    }

    float f32() {
        const uint32_t bits = u32_le();

        if (badbit_)
            return 0.0f;

        return std::bit_cast<float>(bits);
    }

    double f64() {
        const uint64_t bits = u64_le();

        if (badbit_)
            return 0.0;

        return std::bit_cast<double>(bits);
    }

    std::span<const uint8_t> read(size_t len) {
        if (badbit_)
            return {};

        const auto remaining = static_cast<std::size_t>(end_ - cur_);

        if (len > remaining) {
            badbit_ = true;
            return {};
        }

        const uint8_t* start = cur_;
        cur_ += len;

        return {start, len};
    }

    template<size_t NUM>
    void expect(const std::array<uint8_t, NUM>& expected) {
        if (badbit_)
            return;

        const auto remaining = static_cast<std::size_t>(end_ - cur_);
        if (remaining < expected.size()) {
            badbit_ = true;
            return;
        }

        if (!std::equal(expected.begin(), expected.end(), cur_)) {
            badbit_ = true;
            return;
        }

        cur_ += expected.size();
    }

private:
    uint64_t u64_le() {
        auto bytes = read(8);

        if (badbit_)
            return 0;

        return static_cast<uint64_t>(bytes[0])
             | static_cast<uint64_t>(bytes[1]) << 8
             | static_cast<uint64_t>(bytes[2]) << 16
             | static_cast<uint64_t>(bytes[3]) << 24
             | static_cast<uint64_t>(bytes[4]) << 32
             | static_cast<uint64_t>(bytes[5]) << 40
             | static_cast<uint64_t>(bytes[6]) << 48
             | static_cast<uint64_t>(bytes[7]) << 56;
    }

    const uint8_t* begin_;
    const uint8_t* cur_;
    const uint8_t* end_;

    bool badbit_ = false;
};