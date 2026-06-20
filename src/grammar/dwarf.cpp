#include "dwarf.hpp"
#include "grammar/values.hpp"
#include <algorithm>

enum class DebugLineSectionOpcodes {
    copy = 1,
    advance_pc = 2,
    advance_line = 3,
    set_file = 4,
    set_column = 5,
    negate_stmt = 6,
    set_basic_block = 7,
    const_add_pc = 8,
    fixed_advance_pc = 9,
    set_prologue_end = 10
};

enum class DebugLineSectionExtendedOpcodes {
    end_sequence = 1,
    set_address = 2,
    define_file = 3
};

Expected<DebugLineSection> DebugLineSection::parse(ByteCursor& in) {
    std::vector<std::string> src_files;
    std::vector<Segment> segments;

    while (!in.eof()) {
        std::span<const uint8_t> header_bytes = in.read(sizeof(CompilationUnitHeader));
        if (in.bad())
            return Unexpected(ERROR("broken stream"));

        const CompilationUnitHeader* header = reinterpret_cast<const CompilationUnitHeader*>(header_bytes.data());
        size_t header_end = in.offset();

        if (header->version != 2)
            return Unexpected(ERROR(fmt::format(
                "only DWARF version 2 is currently supported (found: {})",
                header->version)));

        Expected<std::vector<std::string>> include_dirs_exp =
            parse_include_dirs(in);
        if (!include_dirs_exp)
            return Unexpected(PROPAGATE(include_dirs_exp));

        Expected<std::vector<std::string>> unit_source_files_exp =
            parse_source_files(in);
        if (!unit_source_files_exp)
            return Unexpected(PROPAGATE(unit_source_files_exp));
        std::vector<std::string>& cu_src_files = *unit_source_files_exp;

        while (true) {
            if (in.offset() + sizeof(CompilationUnitHeader) - header_end >= header->length)
                break;

            State state;
            state.is_stmt = header->default_is_stmt;
            std::vector<State> states;

            while (true) {
                uint8_t op = in.byte();
                if (in.bad())
                    return Unexpected(ERROR("broken stream"));

                if (op == 0) {
                    Expected<bool> is_end_of_seq_exp =
                        handle_extended_opcode(in, state, states);
                    if (!is_end_of_seq_exp)
                        return Unexpected(PROPAGATE(is_end_of_seq_exp));

                    if (*is_end_of_seq_exp)
                        break;
                } else if (op >= 13) {
                    Expected<void> result =
                        handle_special_opcode(op, *header, state, states);
                    if (!result)
                        return Unexpected(PROPAGATE(result));
                } else {
                    Expected<void> result =
                        handle_standard_opcode(in, op, *header, state, states);
                    if (!result)
                        return Unexpected(PROPAGATE(result));
                }
            }

            if (states.size() < 2 || states[0].address >= UINT32_MAX)
                continue;

            for (size_t i = 0; i + 1 < states.size(); ++i) {
                const State& a = states[i];
                const State& b = states[i + 1];

                Segment s;
                s.start_addr = a.address;
                s.end_addr = b.address;
                s.src_file_idx = src_files.size() + static_cast<size_t>(a.file) - 1;
                s.line = a.line;

                segments.push_back(s);
            }
        }

        src_files.insert(src_files.end(), cu_src_files.begin(),
                         cu_src_files.end());
    }

    return DebugLineSection(std::move(segments), std::move(src_files));
}

Expected<std::vector<std::string>>
DebugLineSection::parse_include_dirs(ByteCursor& in) {
    std::vector<std::string> include_dirs;

    std::string directory_buffer;
    while (!in.bad()) {
        char c = static_cast<char>(in.byte());

        if (c == '\0') {
            if (directory_buffer.empty())
                break;

            include_dirs.push_back(directory_buffer);
            directory_buffer.clear();
        } else {
            directory_buffer.push_back(c);
        }
    }

    if (in.bad())
        return Unexpected(ERROR("broken stream"));

    return include_dirs;
}

Expected<std::vector<std::string>>
DebugLineSection::parse_source_files(ByteCursor& in) {
    std::vector<std::string> source_files;

    std::string file_buffer;
    while (!in.bad()) {
        char c = static_cast<char>(in.bad());

        if (c == '\0') {
            if (file_buffer.empty())
                break;

            Expected<U64> dir_idx_exp = U64::parse(in);
            if (!dir_idx_exp)
                return Unexpected(PROPAGATE(dir_idx_exp));

            Expected<U64> time_exp = U64::parse(in);
            if (!time_exp)
                return Unexpected(PROPAGATE(dir_idx_exp));

            Expected<U64> size_exp = U64::parse(in);
            if (!size_exp)
                return Unexpected(PROPAGATE(size_exp));

            source_files.push_back(file_buffer);
            file_buffer.clear();
        } else {
            file_buffer.push_back(c);
        }
    }

    if (!in.bad())
        return Unexpected(ERROR("broken stream"));

    return source_files;
}

Expected<void> DebugLineSection::handle_standard_opcode(
    ByteCursor& in, uint8_t op, const CompilationUnitHeader& header, State& state,
    std::vector<State>& states) {
    switch (static_cast<DebugLineSectionOpcodes>(op)) {
    case DebugLineSectionOpcodes::copy: {
        states.push_back(state);
        break;
    }
    case DebugLineSectionOpcodes::advance_pc: {
        Expected<U64> incr_exp = U64::parse(in);
        if (!incr_exp)
            return Unexpected(PROPAGATE(incr_exp));

        state.address += *incr_exp;
        break;
    }
    case DebugLineSectionOpcodes::advance_line: {
        Expected<S64> incr_exp = S64::parse(in);
        if (!incr_exp)
            return Unexpected(PROPAGATE(incr_exp));

        state.line += *incr_exp;
        break;
    }
    case DebugLineSectionOpcodes::set_file: {
        Expected<U64> file_exp = U64::parse(in);
        if (!file_exp)
            return Unexpected(PROPAGATE(file_exp));

        state.file = static_cast<int64_t>(*file_exp);
        break;
    }
    case DebugLineSectionOpcodes::set_column: {
        Expected<U64> column_exp = U64::parse(in);
        if (!column_exp)
            return Unexpected(PROPAGATE(column_exp));

        state.column = static_cast<int64_t>(*column_exp);
        break;
    }
    case DebugLineSectionOpcodes::negate_stmt:
        break;
    case DebugLineSectionOpcodes::const_add_pc: {
        state.address += ((255 - header.opcode_base) / header.line_range) *
                         header.min_instruction_length;
        break;
    }
    case DebugLineSectionOpcodes::set_prologue_end:
        break;
    default:
        return Unexpected(ERROR(fmt::format("unhandled opcode: {}", op)));
    }

    return {};
}

Expected<void> DebugLineSection::handle_special_opcode(
    uint8_t op, const CompilationUnitHeader& header, State& state,
    std::vector<State>& states) {
    uint8_t adjusted_op = op - header.opcode_base;

    uint64_t advance_addr = adjusted_op / header.line_range;
    int64_t advance_line = header.line_base + (adjusted_op % header.line_range);

    state.address += advance_addr * header.min_instruction_length;
    state.line += advance_line;

    states.push_back(state);
    return {};
}

Expected<bool> DebugLineSection::handle_extended_opcode(
    ByteCursor& in, State& state,
    std::vector<State>& states) {
    Expected<U64> length_exp = U64::parse(in);
    if (!length_exp)
        return Unexpected(PROPAGATE(length_exp));

    uint64_t length = *length_exp;
    uint8_t xop = in.byte();

    switch (static_cast<DebugLineSectionExtendedOpcodes>(xop)) {
    case DebugLineSectionExtendedOpcodes::end_sequence: {
        if (length != 1)
            return Unexpected(ERROR("unexpected extended operation size"));

        states.push_back(state);
        return true;
    }
    case DebugLineSectionExtendedOpcodes::set_address: {
        if (length != 1 + sizeof(uint32_t))
            return Unexpected(
                ERROR(fmt::format("unexpected address size: {}", length)));

        uint32_t address = in.u32_le();
        if (in.bad())
            return Unexpected(ERROR("broken stream"));

        state.address = address;
        return false;
    }
    default:
        return Unexpected(
            ERROR(fmt::format("unhandled extended opcode: {}", xop)));
    }
}
