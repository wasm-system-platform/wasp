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
    case std::to_underlying(Command::seekg):
        seekg(instance, buffer);
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

    std::streampos old_pos = disk_.tellg();
    disk_.read(dest, cmd->count);

    if (!disk_) {
        disk_.clear();
        disk_.seekg(old_pos);
        cmd->result = Result::disk_error;
        return;
    }

    cmd->result = Result::success;
}

void Disk::seekg(Instance& instance, std::span<uint8_t> buffer) {
    if (buffer.size() < sizeof(SeekCommand)) {
        Result* result = reinterpret_cast<Result*>(buffer.data());
        *result = Result::invalid_arguments;
        return;
    }

    std::streampos old_pos = disk_.tellp();

    SeekCommand* cmd = reinterpret_cast<SeekCommand*>(buffer.data());
    switch (cmd->whence) {
    case SeekWhence::set:
        disk_.seekp(cmd->pos, std::ios_base::beg);
        break;
    case SeekWhence::cur:
        disk_.seekp(cmd->pos, std::ios_base::cur);
        break;
    case SeekWhence::end:
        disk_.seekp(cmd->pos, std::ios_base::end);
        break;
    default:
        cmd->result = Result::invalid_arguments;
        return;
    }

    if (!disk_) {
        disk_.clear();
        disk_.seekp(old_pos);
        cmd->result = Result::disk_error;
        return;
    }

    cmd->new_pos = disk_.tellp();
    cmd->result = Result::success;
    return;
}
