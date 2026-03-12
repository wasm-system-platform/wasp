#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "hw/mem/memory_traits.hpp"

#include "runtime/handler.hpp"
#include "runtime/instance.hpp"
#include "runtime/operations.hpp"
#include "util/errno.hpp"

namespace hw::mem {

static constexpr uint32_t WPAGE_SIZE = 1 << UINT16_WIDTH;
static constexpr uint32_t WPAGE_MASK = UINT16_MAX;
static constexpr uint32_t WPAGE_WIDTH = UINT16_WIDTH;
static constexpr uint32_t VIRT_MEMORY = 0x8000'0000;

enum AccessType : uint8_t { READ = 'r', WRITE = 'w' };
enum ProtectionFlags : uint8_t {
    READABLE = 1 << 0,
    WRITABLE = 1 << 1,
};

class MemoryManagementUnit;

class PageTable {
public:
    Errno mapPage(uint32_t virt_addr, uint32_t offset, int32_t prot);
    Errno unmapPage(uint32_t virt_addr, uint32_t* offset_out,
                    int32_t* prot_out);
    Errno getPage(uint32_t virt_addr, uint32_t* offset, int32_t* prot);
    bool translate(uint32_t virt_addr, uint32_t& offset,
                   AccessType access_type);

    void clear() { mapping_.clear(); }

private:
    struct Mapping {
        uint32_t offset;
        int prot;
    };

    std::unordered_map<uint16_t, Mapping> mapping_;
};

class MemoryManagementUnit {
public:
    MemoryManagementUnit(hw::mem::Memory& memory,
                         uint32_t page_fault_handler_idx)
        : memory_(memory),
          page_fault_handler_(
              std::make_shared<runtime::PageFault>(page_fault_handler_idx)) {}

    /* Table management */
    size_t activeTable() const { return active_idx_; }
    Errno createTable(uint32_t& ptable_idx_out);
    Errno destroyTable(uint32_t ptable_idx);
    Errno loadTable(uint32_t ptable_idx);

    Errno mapPage(uint32_t ptable_idx, uint32_t virt_addr, uint32_t offset,
                  int prot);
    Errno unmapPage(uint32_t ptable_idx, uint32_t virt_addr,
                    uint32_t* offset_out, int* prot_out);
    Errno getPage(uint32_t ptable_idx, uint32_t virt_addr, uint32_t* phys_addr,
                  int* prot);

    /* Memory access */
    template <typename T> Errno ptr(uint32_t virt_addr, T** ptr_out);

    template <Scalar T> bool load(uint32_t virt_addr, T& value_out) {
        uint32_t offset0;

        if (!tables_[active_idx_].translate(virt_addr, offset0,
                                            AccessType::READ))
            return false;

        uint32_t page_offset = virt_addr & WPAGE_MASK;

        // single page access
        if (page_offset + sizeof(T) <= WPAGE_SIZE) {
            return memory_.load(offset0, value_out);
        }

        // --- split over two pages ---

        uint32_t virt_addr2 = virt_addr + (WPAGE_SIZE - page_offset);
        uint32_t offset1;

        if (!tables_[active_idx_].translate(virt_addr2, offset1,
                                            AccessType::READ))
            return false;

        uint8_t* ptr0;
        uint8_t* ptr1;

        memory_.ptr(offset0, &ptr0);
        memory_.ptr(offset1, &ptr1);

        uint32_t first_size = WPAGE_SIZE - page_offset;
        uint32_t second_size = sizeof(T) - first_size;

        uint8_t* dst = reinterpret_cast<uint8_t*>(&value_out);
        std::memcpy(dst, ptr0, first_size);
        std::memcpy(dst + first_size, ptr1, second_size);

        return true;
    }

    template <ContiguousBuffer T> bool load(uint32_t virt_addr, T& dst_buffer) {
        uint8_t* dst_ptr = dst_buffer.data();
        uint32_t remaining = dst_buffer.size();
        uint32_t curr_addr = virt_addr;

        while (remaining > 0) {
            uint32_t offset;
            if (!tables_[active_idx_].translate(curr_addr, offset,
                                                AccessType::READ))
                return false;

            uint32_t page_offset = curr_addr & WPAGE_MASK;
            uint32_t chunk_size = std::min(remaining, WPAGE_SIZE - page_offset);

            uint8_t* src_ptr;
            memory_.ptr(offset, &src_ptr);

            std::memcpy(dst_ptr, src_ptr, chunk_size);

            dst_ptr += chunk_size;
            curr_addr += chunk_size;
            remaining -= chunk_size;
        }

        return true;
    }

    template <Scalar T> bool store(uint32_t virt_addr, T value) {
        uint32_t offset0;

        if (!tables_[active_idx_].translate(virt_addr, offset0,
                                            AccessType::WRITE))
            return false;

        uint32_t page_offset = virt_addr & WPAGE_MASK;

        // single page access
        if (page_offset + sizeof(T) <= WPAGE_SIZE) {
            return memory_.store(offset0, value);
        }

        // --- split over two pages ---

        uint32_t virt_addr2 = virt_addr + (WPAGE_SIZE - page_offset);
        uint32_t offset1;

        if (!tables_[active_idx_].translate(virt_addr2, offset1,
                                            AccessType::WRITE))
            return false;

        uint8_t* ptr0;
        uint8_t* ptr1;

        memory_.ptr(offset0, &ptr0);
        memory_.ptr(offset1, &ptr1);

        uint32_t first_size = WPAGE_SIZE - page_offset;
        uint32_t second_size = sizeof(T) - first_size;

        uint8_t* src = reinterpret_cast<uint8_t*>(&value);
        std::memcpy(ptr0, src, first_size);
        std::memcpy(ptr1, src + first_size, second_size);

        return true;
    }

    template <ContiguousBuffer T>
    bool store(uint32_t virt_addr, const T& src_buffer) {
        const uint8_t* src_ptr = src_buffer.data();
        uint32_t remaining = src_buffer.size();
        uint32_t curr_addr = virt_addr;

        while (remaining > 0) {
            uint32_t offset;
            if (!tables_[active_idx_].translate(curr_addr, offset,
                                                AccessType::WRITE))
                return false;

            uint32_t page_offset = curr_addr & WPAGE_MASK;
            uint32_t chunk_size = std::min(remaining, WPAGE_SIZE - page_offset);

            uint8_t* dst_ptr;
            memory_.ptr(offset, &dst_ptr);

            std::memcpy(dst_ptr, src_ptr, chunk_size);

            src_ptr += chunk_size;
            curr_addr += chunk_size;
            remaining -= chunk_size;
        }

        return true;
    }

    bool fill(uint32_t virt_addr, uint8_t value, uint32_t count) {
        uint32_t remaining = count;
        uint32_t curr_addr = virt_addr;

        while (remaining > 0) {
            uint32_t offset;
            if (!tables_[active_idx_].translate(curr_addr, offset,
                                                AccessType::WRITE))
                return false;

            uint32_t page_offset = curr_addr & WPAGE_MASK;
            uint32_t chunk_size = std::min(remaining, WPAGE_SIZE - page_offset);

            uint8_t* dst_ptr;
            memory_.ptr(offset, &dst_ptr);

            std::memset(dst_ptr, value, chunk_size);

            curr_addr += chunk_size;
            remaining -= chunk_size;
        }

        return true;
    }

    // Errno checkAccess(uint32_t virt_addr, AccessType access_type);

    runtime::Continuation fault(runtime::Instance& instance, uint32_t addr,
                                bool is_write);

private:
    hw::mem::Memory& memory_;

    std::vector<PageTable> tables_ = {{}};
    std::stack<uint32_t> free_list_;

    uint32_t active_idx_ = 0;

    runtime::Operation page_fault_handler_;
};

} // namespace hw::mem
