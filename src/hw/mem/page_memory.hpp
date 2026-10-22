#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <span>
#include <vector>

#include "hw/mem/memory_traits.hpp"

namespace hw {

namespace mem {

class PageMemory {
public:
    PageMemory(uint16_t num_pages) { grow(num_pages); }

    template <typename T> T* ptr(uint32_t offset) {
        const size_t page_idx = offset / WASM_PAGE_SIZE;
        const size_t page_offset = offset % WASM_PAGE_SIZE;

        Page& page = writable_page(page_idx);

        return reinterpret_cast<T*>(page.data() + page_offset);
    }

    template <Scalar T> T load(uint32_t offset) {
        T value;
        read_bytes(offset, std::as_writable_bytes(std::span(&value, 1)));
        return value;
    }

    template <ContiguousBuffer T> void load(uint32_t offset, T& dst_buffer) {
        read_bytes(offset, std::as_writable_bytes(std::span{
                               dst_buffer.data(), dst_buffer.size()}));
    }

    template <Scalar T> void store(uint32_t offset, T value) {
        write_bytes(offset, std::as_bytes(std::span{&value, 1}));
    }

    template <ContiguousBuffer T>
    void store(uint32_t offset, const T& src_buffer) {
        write_bytes(offset, std::as_bytes(std::span{src_buffer.data(),
                                                    src_buffer.size()}));
    }

    void store(uint32_t offset, std::span<const uint8_t> data) {
        write_bytes(offset, std::as_bytes(data));
    }

    void fill(uint32_t offset, uint8_t value, uint32_t count) {
        size_t pos = offset;
        size_t remaining = count;

        while (remaining > 0) {
            const size_t page_idx = pos / WASM_PAGE_SIZE;
            const size_t page_offset = pos % WASM_PAGE_SIZE;
            const size_t n = std::min(remaining, WASM_PAGE_SIZE - page_offset);

            Page& page = writable_page(page_idx);

            std::memset(page.data() + page_offset, value, n);

            remaining -= n;
            pos += n;
        }
    }

    void grow(uint32_t delta) {
        static std::shared_ptr<Page> zero_page = std::make_shared<Page>();

        pages_.reserve(pages_.size() + delta);
        for (uint32_t i = 0; i < delta; ++i) {
            pages_.push_back(zero_page);
        }
    }

private:
    using Page = std::array<std::byte, WASM_PAGE_SIZE>;

    void read_bytes(uint32_t offset, std::span<std::byte> dst) const {
        size_t pos = offset;
        size_t done = 0;

        while (done < dst.size()) {
            const size_t page_idx = pos / WASM_PAGE_SIZE;
            const size_t page_offset = pos % WASM_PAGE_SIZE;
            const size_t n =
                std::min(dst.size() - done, WASM_PAGE_SIZE - page_offset);

            std::memcpy(dst.data() + done,
                        pages_[page_idx]->data() + page_offset, n);

            done += n;
            pos += n;
        }
    }

    void write_bytes(uint32_t offset, std::span<const std::byte> src) {
        size_t pos = offset;
        size_t done = 0;

        while (done < src.size()) {
            const size_t page_idx = pos / WASM_PAGE_SIZE;
            const size_t page_offset = pos % WASM_PAGE_SIZE;
            const size_t n =
                std::min(src.size() - done, WASM_PAGE_SIZE - page_offset);

            Page& page = writable_page(page_idx);

            std::memcpy(page.data() + page_offset, src.data() + done, n);

            done += n;
            pos += n;
        }
    }

    Page& writable_page(size_t index) {
        auto& page = pages_[index];

        if (page.use_count() > 1) {
            page = std::make_shared<Page>(*page);
        }

        return *page;
    }

    std::vector<std::shared_ptr<Page>> pages_;
    std::vector<uint8_t> data_;
};

} // namespace mem

} // namespace hw
