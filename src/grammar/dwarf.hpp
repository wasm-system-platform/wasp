#pragma once

#include "util/byte_cursor.hpp"
#include "util/error_handling.hpp"
#include <istream>

class DebugLineSection {
public:
    struct Segment {
        size_t start_addr;
        size_t end_addr;
        size_t src_file_idx;
        int64_t line;
    };

    DebugLineSection() = default;

    static Expected<DebugLineSection> parse(ByteCursor& in);

    const std::vector<Segment>& getSegments() const { return segments_; }
    const std::vector<std::string>& getSourceFiles() const {
        return src_files_;
    }

private:
    struct CompilationUnitHeader {
        uint32_t length;
        uint16_t version;
        uint32_t header_length;
        uint8_t min_instruction_length;
        uint8_t default_is_stmt;
        int8_t line_base;
        uint8_t line_range;
        uint8_t opcode_base;
        uint8_t std_opcode_lengths[12];
    } __attribute__((packed));

    struct State {
        uint64_t address = 0;
        int64_t file = 1;
        int64_t line = 1;
        int64_t column = 0;
        bool is_stmt;
    };

    std::vector<Segment> segments_;
    std::vector<std::string> src_files_;

    DebugLineSection(std::vector<Segment>&& segments,
                     std::vector<std::string>&& src_files)
        : segments_(std::move(segments)), src_files_(std::move(src_files)) {}

    static Expected<std::vector<std::string>>
    parse_include_dirs(ByteCursor& in);
    static Expected<std::vector<std::string>>
    parse_source_files(ByteCursor& in);

    static Expected<void> handle_standard_opcode(ByteCursor& in, uint8_t op,
                                                 const CompilationUnitHeader& header,
                                                 State& state,
                                                 std::vector<State>& states);
    static Expected<void> handle_special_opcode(uint8_t op,
                                                const CompilationUnitHeader& header,
                                                State& state,
                                                std::vector<State>& states);
    static Expected<bool> handle_extended_opcode(ByteCursor& in,
                                                 State& state,
                                                 std::vector<State>& states);
};
