#include <utility>

#include "devices/disk.hpp"
#include "runtime/instance.hpp"

Expected<Device> Disk::create(const std::string& path) {
    std::fstream disk(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!disk)
        return Unexpected(ERROR(fmt::format("Failed to open file: {}", path)));

    class Builder : public Disk {
    public:
        Builder(std::fstream&& disk) : Disk(std::move(disk)) {}
    };

    return std::make_shared<Builder>(std::move(disk));
}

Disk::Disk(std::fstream&& disk)
    : DeviceBase(VENDOR_ID, DEVICE_ID), disk_(std::move(disk)) {}

void Disk::io(Instance& instance, int32_t cmd, std::span<uint8_t> buffer) {
    switch (cmd) {
    case std::to_underlying(Command::read):
        read(instance, buffer);
        break;
    case std::to_underlying(Command::write):
        write(instance, buffer);
        break;
    case std::to_underlying(Command::flush):
        flush(instance, buffer);
        break;
    case std::to_underlying(Command::get_size):
        getSize(instance, buffer);
        break;
    default:
        fmt::println("unknown cmd: {}", cmd);
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
    }
}

void Disk::read(Instance& instance, std::span<uint8_t> buffer) {
    struct ReadCommand {
        Result result;
        uint32_t dest;
        uint32_t count;
        uint64_t offset;
    } __attribute__((packed));

    if (buffer.size() < sizeof(ReadCommand)) {
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
        return;
    }

    ReadCommand* cmd = reinterpret_cast<ReadCommand*>(buffer.data());

    auto& memory = instance.getGlobalState().getMemory();
    if (!memory.contains(cmd->dest, cmd->count)) {
        cmd->result = Result::invalid_arguments;
        return;
    }

    char* dest;
    memory.ptr(cmd->dest, &dest);

    disk_.clear();

    disk_.seekg(cmd->offset, std::ios_base::beg);
    if (!disk_) {
        cmd->result = Result::disk_error;
        return;
    }

    disk_.read(dest, cmd->count);
    cmd->result = disk_ ? Result::success : Result::disk_error;
}

void Disk::write(Instance& instance, std::span<uint8_t> buffer) {
    struct WriteCommand {
        Result result;
        uint32_t src;
        uint32_t count;
        uint64_t offset;
    } __attribute__((packed));

    if (buffer.size() < sizeof(WriteCommand)) {
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
        return;
    }

    WriteCommand* cmd = reinterpret_cast<WriteCommand*>(buffer.data());

    auto& memory = instance.getGlobalState().getMemory();
    if (!memory.contains(cmd->src, cmd->count)) {
        cmd->result = Result::invalid_arguments;
        return;
    }

    const char* src;
    memory.ptr(cmd->src, &src);

    disk_.clear();

    disk_.seekp(cmd->offset, std::ios_base::beg);
    if (!disk_) {
        cmd->result = Result::disk_error;
        return;
    }

    disk_.write(src, cmd->count);
    cmd->result = disk_ ? Result::success : Result::disk_error;
};

void Disk::flush(Instance& instance, std::span<uint8_t> buffer) {
    Result* result = reinterpret_cast<Result*>(buffer.data());
    *result = disk_ ? Result::success : Result::disk_error;
}

void Disk::getSize(Instance& instance, std::span<uint8_t> buffer) {
    struct GetSizeCommand {
        Result result;
        uint64_t size;
    } __attribute__((packed));

    if (buffer.size() < sizeof(GetSizeCommand)) {
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
        return;
    }

    GetSizeCommand* cmd = reinterpret_cast<GetSizeCommand*>(buffer.data());

    disk_.clear();
    disk_.seekg(0, std::ios_base::end);

    if (disk_) {
        const auto position = disk_.tellg();

        if (position != std::fstream::pos_type(-1)) {
            const auto size = static_cast<std::streamoff>(position);

            if (size >= 0) {
                cmd->size = static_cast<uint64_t>(size);
                cmd->result = Result::success;
            }
        }
    }
}
