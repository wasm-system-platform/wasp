#include <fstream>

#include "hw/wasm_disk.hpp"

Expected<WasmDisk> WasmDisk::create(const std::string& path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file)
        return Unexpected(ERROR(fmt::format("Failed to open file: {}", path)));

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<uint8_t> buffer(size);
    if (!file.read(reinterpret_cast<char*>(buffer.data()), size))
        return Unexpected(ERROR(fmt::format("Failed to read file: {}", path)));

    return WasmDisk(std::move(buffer));
}

void WasmDisk::read(void* dst, uint32_t offset, uint32_t count) {
    std::memcpy(dst, &data_[offset], count);
}

void WasmDisk::write(void* src, uint32_t offset, uint32_t count) {
    std::memcpy(&data_[offset], src, count);
}

WasmDisk::WasmDisk(std::vector<uint8_t>&& data) : data_(std::move(data)) {}
