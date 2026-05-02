#pragma once

#include <cstdint>

#include "file_memory.hpp"
#include "vector_memory.hpp"

namespace hw::mem {

template <typename Impl> class BasicMemory {
public:
    BasicMemory(uint16_t num_init_pages, uint16_t num_max_pages)
        : num_max_pages_(num_max_pages), num_curr_pages_(num_init_pages),
          impl(num_init_pages) {}

    bool contains(uint32_t offset, uint32_t len) {
        uint32_t size = num_curr_pages_ << UINT16_WIDTH;
        return offset <= size && len <= size - offset;
    }

    template <typename T> bool ptr(uint32_t offset, T** ptr_out) {
        if (!contains(offset, sizeof(T)))
            return false;

        *ptr_out = impl.template ptr<T>(offset);
        return true;
    }

    template <Scalar T> bool load(uint32_t offset, T& value_out) {
        if (!contains(offset, sizeof(T)))
            return false;

        value_out = impl.template load<T>(offset);
        return true;
    }

    template <ContiguousBuffer T> bool load(uint32_t offset, T& dst_buffer) {
        if (!contains(offset, dst_buffer.size()))
            return false;

        impl.load(offset, dst_buffer);
        return true;
    }

    template <Scalar T> bool store(uint32_t offset, T value) {
        if (!contains(offset, sizeof(T)))
            return false;

        impl.store(offset, value);
        return true;
    }

    template <ContiguousBuffer T>
    bool store(uint32_t offset, const T& src_buffer) {
        if (!contains(offset, sizeof(T)))
            return false;

        impl.store(offset, src_buffer);
        return true;
    }

    bool fill(uint32_t offset, uint8_t value, uint32_t count) {
        if (!contains(offset, count))
            return false;

        impl.fill(offset, value, count);
        return true;
    }

    uint32_t grow(uint32_t delta) {
        if (delta == 0)
            return num_curr_pages_;

        uint32_t num_pages = num_curr_pages_ + delta;

        if ((num_pages > num_max_pages_) ||
            (num_pages > 1 << (UINT16_WIDTH - 1)))
            return UINT32_MAX;

        uint32_t num_old_pages = num_curr_pages_;
        num_curr_pages_ += delta;

        impl.grow(delta);
        return num_old_pages;
    }

    void updateLimit(uint16_t limit) { num_max_pages_ = limit; }

private:
    uint16_t num_max_pages_;
    uint16_t num_curr_pages_;

    Impl impl;
};

#if defined(MEMORY_VECTOR)
using Memory = BasicMemory<VectorMemory>;
#elif defined(MEMORY_FILE)
using Memory = BasicMemory<FileMemory>;
#else
// auto
using Memory = BasicMemory<VectorMemory>;
#endif

} // namespace hw::mem
