#pragma once

#include <cstdint>

#include "util/error_handling.hpp"

/*********/
/* Bytes */
/*********/

struct Byte {
    static Expected<Byte> parse(std::istream& in);

    operator uint8_t() const { return val_; }

private:
    uint8_t val_;

    explicit Byte(uint8_t val) : val_(val) {}
};

/************/
/* Integers */
/************/

struct S32 {
    static Expected<S32> parse(std::istream& in);

    operator int32_t() const { return val_; }

private:
    int32_t val_;

    explicit S32(int32_t val) : val_(val) {}
};

struct U32 {
    static Expected<U32> parse(std::istream& in);

    operator uint32_t() const { return val_; }

private:
    uint32_t val_;

    explicit U32(uint32_t val) : val_(val) {}
};

struct S64 {
    static Expected<S64> parse(std::istream& in);

    operator int64_t() const { return val_; }

private:
    int64_t val_;

    explicit S64(int64_t val) : val_(val) {}
};

struct U64 {
    static Expected<U64> parse(std::istream& in);

    operator uint64_t() const { return val_; }

private:
    uint64_t val_;

    explicit U64(uint64_t val) : val_(val) {}
};

/*********/
/* Names */
/*********/

struct Name {
    static Expected<Name> parse(std::istream& in);

    operator const std::string&() const { return val_; }

private:
    std::string val_;

    explicit Name(std::string&& val) : val_(std::move(val)) {}
};
