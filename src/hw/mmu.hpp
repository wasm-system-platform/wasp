#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "util/error_handling.hpp"
#include "util/errno.hpp"

static constexpr uint32_t VIRT_MEMORY = 0x8000'0000;

enum AccessType : uint8_t { READ = 'r', WRITE = 'w' };
enum ProtectionFlags : uint8_t {
    READABLE = 1 << 0,
    WRITABLE = 1 << 1,
};

class PageTable {
public:
    Errno mapPage(uint32_t va, uint8_t* page, int prot);
    Errno unmapPage(uint32_t va, uint8_t** pa, int* prot);
    Errno getPage(uint32_t va, uint8_t** pa, int* prot);
    Errno translate(uint32_t va, uint8_t** pa, AccessType access_type);

private:
    struct Mapping {
        uint8_t* page;
        int prot;
    };

    std::unordered_map<uint16_t, Mapping> mapping_;
};

class MemoryManagementUnit {
public:
    static MemoryManagementUnit& instance();
    
    /* Table management */
    size_t activeTable() const { return active_idx_; }
    size_t createTable();
    Errno loadTable(size_t table_idx);

    Errno mapPage(size_t table_idx, uint32_t va, uint8_t* page, int prot);
    Errno unmapPage(size_t table_idx, uint32_t va, uint8_t** pa, int* prot);
    Errno getPage(size_t table_idx, uint32_t va, uint8_t** pa, int* prot);
    
    /* Memory access */
    Errno read(uint32_t va, std::vector<uint8_t>& dst);
    Errno readI8(uint32_t va, int8_t* value);
    Errno readI16(uint32_t va, int16_t* value);
    Errno readI32(uint32_t va, int32_t* value);
    Errno readI64(uint32_t va, int64_t* value);

    Errno write(uint32_t va, const std::vector<uint8_t>& src);
    Errno writeI8(uint32_t va, int8_t value);
    Errno writeI16(uint32_t va, int16_t value);
    Errno writeI32(uint32_t va, int32_t value);
    Errno writeI64(uint32_t va, int64_t value);

    Errno memset(uint32_t va, uint8_t value, size_t count);

    Errno checkAccess(uint32_t va, AccessType access_type);

private:
    std::vector<PageTable> tables_;
    size_t active_idx_ = 0;

    MemoryManagementUnit();
};
