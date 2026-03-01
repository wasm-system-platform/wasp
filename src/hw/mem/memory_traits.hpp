#pragma once

#include <type_traits>

namespace hw::mem {

template<typename T>
concept Scalar = std::is_arithmetic_v<T>;

template<typename T>
concept ContiguousBuffer =
    requires(T v) {
        typename T::value_type;
        { v.data() } -> std::same_as<uint8_t*>;
        { v.size() } -> std::convertible_to<std::size_t>;
    };

}
