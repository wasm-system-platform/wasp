#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "hw/mem/memory_traits.hpp"

namespace hw {

namespace mem {

class VectorMemory {
public:
    VectorMemory(uint16_t num_pages) : data_(num_pages * 65'536) {}

    template <typename T> T* ptr(uint32_t offset) {
        return reinterpret_cast<T*>(data_.data() + offset);
    }

    template <Scalar T> T load(uint32_t offset) {
        return *reinterpret_cast<T*>(data_.data() + offset);
    }

    template <ContiguousBuffer T> void load(uint32_t offset, T& dst_buffer) {
        std::memcpy(dst_buffer.data(), data_.data() + offset,
                    dst_buffer.size());
    }

    template <Scalar T> void store(uint32_t offset, T value) {
        *reinterpret_cast<T*>(data_.data() + offset) = value;
    }

    template <ContiguousBuffer T>
    void store(uint32_t offset, const T& src_buffer) {
        std::memcpy(data_.data() + offset, src_buffer.data(),
                    src_buffer.size());
    }

    void fill(uint32_t offset, uint8_t value, uint32_t count) {
        std::memset(data_.data() + offset, value, count);
    }

    void grow(uint32_t delta) {
        size_t new_size = data_.size() + (delta << 16);
        data_.resize(new_size);
    }

private:
    std::vector<uint8_t> data_;
};

} // namespace mem

} // namespace hw
