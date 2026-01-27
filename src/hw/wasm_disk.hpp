#pragma once

#include <cstdint>
#include <fstream>
#include <vector>

#include "util/error_handling.hpp"

class WasmDisk {
public:
    static Expected<WasmDisk> create(const std::string& path);

    // input
    int read(void* dst, uint32_t count);
    int tellg(uint32_t* offset);
    int seekg(int32_t offset, int whence, uint32_t* new_offset);

    // output
    int write(void* src, uint32_t count);
    int tellp(uint32_t* offset);
    int seekp(int32_t offset, int whence, uint32_t* new_offset);

private:
    std::fstream disk_;

    WasmDisk(std::fstream&& disk);
};