#include <fstream>

#include "hw/wasm_disk.hpp"

Expected<WasmDisk> WasmDisk::create(const std::string& path) {
    std::fstream disk(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk)
        return Unexpected(ERROR(fmt::format("Failed to open file: {}", path)));

    disk.seekg(0, std::ios_base::end);
    std::streampos size = disk.tellg();
    if (size == std::streampos(-1))
        return Unexpected(
            ERROR(fmt::format("Failed to get file size: {}", path)));

    if (size > 0xFFFFFFFF)
        return Unexpected(
            ERROR(fmt::format("File too large (>4GB): {}", path)));

    disk.seekg(0, std::ios_base::beg);

    return WasmDisk(std::move(disk));
}

int WasmDisk::read(void* dst, uint32_t count) {
    std::streampos old_pos = disk_.tellg();
    disk_.read(reinterpret_cast<char*>(dst), count);

    if (!disk_) {
        disk_.clear();
        disk_.seekg(old_pos);

        return -1;
    };

    return 0;
}

int WasmDisk::tellg(uint32_t* offset) {
    std::streampos pos = disk_.tellg();
    if (pos == std::streampos(-1)) {
        return -1;
    }

    *offset = static_cast<uint32_t>(pos);
    return 0;
}

int WasmDisk::seekg(int32_t pos, int whence, uint32_t* new_pos) {
    std::streampos old_pos = disk_.tellg();

    switch (whence) {
    case SEEK_SET:
        disk_.seekg(pos, std::ios_base::beg);
        break;
    case SEEK_CUR:
        disk_.seekg(pos, std::ios_base::cur);
        break;
    case SEEK_END:
        disk_.seekg(pos, std::ios_base::end);
        break;
    default:
        return -1;
    }

    if (!disk_) {
        disk_.clear();
        disk_.seekg(old_pos);
        return -1;
    }

    *new_pos = static_cast<uint32_t>(disk_.tellg());
    return 0;
}

int WasmDisk::write(void* src, uint32_t count) {
    std::streampos old_pos = disk_.tellp();
    disk_.write(reinterpret_cast<char*>(src), count);

    if (!disk_) {
        disk_.clear();
        disk_.seekp(old_pos);

        return -1;
    };

    return 0;
}

int WasmDisk::tellp(uint32_t* offset) {
    std::streampos pos = disk_.tellp();
    if (pos == std::streampos(-1)) {
        return -1;
    }

    *offset = static_cast<uint32_t>(pos);
    return 0;
}

int WasmDisk::seekp(int32_t offset, int whence, uint32_t* new_offset) {
    std::streampos old_pos = disk_.tellp();

    switch (whence) {
    case 0:
        disk_.seekp(offset, std::ios_base::beg);
        break;
    case 1:
        disk_.seekp(offset, std::ios_base::cur);
        break;
    case 2:
        disk_.seekp(offset, std::ios_base::end);
        break;
    default:
        return -1;
    }

    if (!disk_) {
        disk_.clear();
        disk_.seekp(old_pos);
        return -1;
    }

    *new_offset = static_cast<uint32_t>(disk_.tellp());
    return 0;
}

WasmDisk::WasmDisk(std::fstream&& disk) : disk_(std::move(disk)) {}
