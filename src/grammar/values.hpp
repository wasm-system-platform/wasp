#pragma once

#include <cstdint>

#include "util/byte_cursor.hpp"
#include "util/error_handling.hpp"

/************/
/* Integers */
/************/

struct S32 {
    static Expected<S32> parse(ByteCursor& in);

    operator int32_t() const { return val_; }

private:
    int32_t val_;

    explicit S32(int32_t val) : val_(val) {}
};

struct U32 {
    static Expected<U32> parse(ByteCursor& in);

    operator uint32_t() const { return val_; }

private:
    uint32_t val_;

    explicit U32(uint32_t val) : val_(val) {}
};

struct S64 {
    static Expected<S64> parse(ByteCursor& in);

    operator int64_t() const { return val_; }

private:
    int64_t val_;

    explicit S64(int64_t val) : val_(val) {}
};

struct U64 {
    static Expected<U64> parse(ByteCursor& in);

    operator uint64_t() const { return val_; }

private:
    uint64_t val_;

    explicit U64(uint64_t val) : val_(val) {}
};

/******************/
/* Floating-Point */
/******************/

struct F32 {
public:
    static Expected<F32> parse(ByteCursor& in);

    float getVal() const { return val_; }

private:
    float val_;

    explicit F32(float val) : val_(val) {}
};

struct F64 {
    static Expected<F64> parse(ByteCursor& in);

    double getVal() const { return val_; }

private:
    double val_;

    explicit F64(double val) : val_(val) {}
};

/*********/
/* Names */
/*********/

struct Name {
    static Expected<Name> parse(ByteCursor& in);

    std::string_view value() const { return val_; }

private:
    std::string_view val_;

    explicit Name(std::string_view val) : val_(val) {}
};
