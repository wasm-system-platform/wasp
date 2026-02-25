#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "runtime/handler.hpp"
#include "runtime/operations.hpp"
#include "util/errno.hpp"

static constexpr uint32_t VIRT_MEMORY = 0x8000'0000;

enum AccessType : uint8_t { READ = 'r', WRITE = 'w' };
enum ProtectionFlags : uint8_t {
    READABLE = 1 << 0,
    WRITABLE = 1 << 1,
};

class PageTable {
public:
    Errno mapPage(uint32_t virt_addr, uint32_t phys_addr, uint8_t* real_addr, int prot);
    Errno unmapPage(uint32_t virt_addr, uint32_t* phys_addr, int* prot);
    Errno getPage(uint32_t virt_addr, uint32_t* phys_addr, int* prot);
    Errno translate(uint32_t virt_addr, uint8_t** real_addr, AccessType access_type);

private:
    struct Mapping {
        uint32_t phys_addr;
        uint8_t* real_addr;
        int prot;
    };

    std::unordered_map<uint16_t, Mapping> mapping_;
};

class MemoryManagementUnit {
public:
    MemoryManagementUnit(uint32_t page_fault_handler_idx) : page_fault_handler_(std::make_shared<runtime::PageFault>(page_fault_handler_idx)) {}

    /* Table management */
    size_t activeTable() const { return active_idx_; }
    Errno createTable(uint32_t* ptable_idx);
    Errno loadTable(uint32_t ptable_idx);

    Errno mapPage(uint32_t ptable_idx, uint32_t virt_addr, uint32_t phys_addr, uint8_t* real_addr, int prot);
    Errno unmapPage(uint32_t ptable_idx, uint32_t virt_addr, uint32_t* phys_addr, int* prot);
    Errno getPage(uint32_t ptable_idx, uint32_t virt_addr, uint32_t* phys_addr, int* prot);
    
    /* Memory access */
    Errno read(uint32_t virt_addr, std::vector<uint8_t>& dst);
    Errno readI8(uint32_t virt_addr, int8_t* value);
    Errno readI16(uint32_t virt_addr, int16_t* value);
    Errno readI32(uint32_t virt_addr, int32_t* value);
    Errno readI64(uint32_t virt_addr, int64_t* value);

    Errno write(uint32_t virt_addr, const std::vector<uint8_t>& src);
    Errno writeI8(uint32_t virt_addr, int8_t value);
    Errno writeI16(uint32_t virt_addr, int16_t value);
    Errno writeI32(uint32_t virt_addr, int32_t value);
    Errno writeI64(uint32_t virt_addr, int64_t value);

    Errno memset(uint32_t virt_addr, uint8_t value, size_t count);

    Errno checkAccess(uint32_t virt_addr, AccessType access_type);

    runtime::Continuation fault(runtime::Instance& instance, uint32_t addr, bool is_write);

private:
    std::vector<PageTable> tables_ = {{}};
    uint32_t active_idx_ = 0;

    runtime::Operation page_fault_handler_;
};
