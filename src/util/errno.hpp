#pragma once

#include <cstdint>


enum class Errno : int32_t {
    SUCCESS = 0,
    INVALID_ARGUMENT = 1,
    OUT_OF_MEMORY = 2,
    EXIST = 3,
    BAD_ADDRESS = 4,
    PERMISSION_DENIED = 5,
    NAME_TOO_LONG = 36,
};
