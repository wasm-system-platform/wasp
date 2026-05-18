#pragma once

#include <cstdint>

enum class Errno : int32_t {
    success = 0,
    invalid = 1,
    access = 2,
    EXIST = 3,
    BAD_ADDRESS = 4,
    PERMISSION_DENIED = 5,
    BUFFER_TO_SMALL = 6,
    NO_DATA = 7,
    io = 29,
    NAME_TOO_LONG = 36,
    no_memory = 48,
    overflow = 61,
    unknown = 79,
};
