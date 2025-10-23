#pragma once

#include <cstdint>
#include <vector>

#include "util/error_handling.hpp"

class WasmDisk {
public:
    static Expected<WasmDisk> create(const std::string& path);

    void read(void* dst, uint32_t offset, uint32_t count);
    void write(void* src, uint32_t offset, uint32_t count);

    uint32_t size() const { return data_.size(); }

private:
    std::vector<uint8_t> data_;

    WasmDisk(std::vector<uint8_t>&& data);
};