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

Expected<DebugLineSection> DebugLineSection::parse(std::istream& in) {
    std::vector<std::string> src_files;
    std::vector<Segment> segments;

    while (in.peek() != EOF) {
        CompilationUnitHeader header;
        if (!in.read(reinterpret_cast<char*>(&header),
                     sizeof(CompilationUnitHeader)))
            return Unexpected(ERROR("unexpected end of section"));

        size_t header_end = in.tellg();

        if (header.version != 2)
            return Unexpected(
                ERROR(fmt::format("only DWARF version 2 is currently supported (found: {})",
                                  header.version)));

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
            if (static_cast<size_t>(in.tellg()) +
                    sizeof(CompilationUnitHeader) - header_end >=
                header.length)
                break;

            State state;
            state.is_stmt = header.default_is_stmt;
            std::vector<State> states;

            while (true) {
                Expected<Byte> op_exp = Byte::parse(in);
                if (!op_exp)
                    return Unexpected(PROPAGATE(op_exp));

                uint8_t op = *op_exp;

                if (op == 0) {
                    Expected<bool> is_end_of_seq_exp =
                        handle_extended_opcode(in, op, header, state, states);
                    if (!is_end_of_seq_exp)
                        return Unexpected(PROPAGATE(is_end_of_seq_exp));

                    if (*is_end_of_seq_exp)
                        break;
                } else if (op >= 13) {
                    Expected<void> result =
                        handle_special_opcode(in, op, header, state, states);
                    if (!result)
                        return Unexpected(PROPAGATE(result));
                } else {
                    Expected<void> result =
                        handle_standard_opcode(in, op, header, state, states);
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
                s.src_file_idx = src_files.size() + a.file - 1;
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
DebugLineSection::parse_include_dirs(std::istream& in) {
    std::vector<std::string> include_dirs;

    std::string directory_buffer;
    while (true) {
        Expected<Byte> c_exp = Byte::parse(in);
        if (!c_exp)
            return Unexpected(PROPAGATE(c_exp));

        char c = *c_exp;

        if (c == '\0') {
            if (directory_buffer.empty())
                break;

            include_dirs.push_back(directory_buffer);
            directory_buffer.clear();
        } else {
            directory_buffer.push_back(c);
        }
    }

    return include_dirs;
}

Expected<std::vector<std::string>>
DebugLineSection::parse_source_files(std::istream& in) {
    std::vector<std::string> source_files;

    std::string file_buffer;
    while (true) {
        Expected<Byte> c_exp = Byte::parse(in);
        if (!c_exp)
            return Unexpected(PROPAGATE(c_exp));

        char c = *c_exp;

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

            // int64_t dir_idx = *dir_idx_exp;
            // int64_t time = *time_exp;
            // int64_t size = *size_exp;

            source_files.push_back(file_buffer);
            file_buffer.clear();
        } else {
            file_buffer.push_back(c);
        }
    }

    return source_files;
}

Expected<void> DebugLineSection::handle_standard_opcode(
    std::istream& in, uint8_t op, CompilationUnitHeader& header, State& state,
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

        state.file = *file_exp;
        break;
    }
    case DebugLineSectionOpcodes::set_column: {
        Expected<U64> column_exp = U64::parse(in);
        if (!column_exp)
            return Unexpected(PROPAGATE(column_exp));

        state.column = *column_exp;
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
    std::istream& in, uint8_t op, CompilationUnitHeader& header, State& state,
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
    std::istream& in, uint8_t op, CompilationUnitHeader& header, State& state,
    std::vector<State>& states) {
    Expected<U64> length_exp = U64::parse(in);
    if (!length_exp)
        return Unexpected(PROPAGATE(length_exp));

    uint64_t length = *length_exp;

    Expected<Byte> eop_exp = Byte::parse(in);
    if (!eop_exp)
        return Unexpected(PROPAGATE(eop_exp));

    uint8_t eop = *eop_exp;

    switch (static_cast<DebugLineSectionExtendedOpcodes>(eop)) {
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

        uint32_t address;
        in.read(reinterpret_cast<char*>(&address), sizeof(uint32_t));

        state.address = address;
        return false;
    }
    default:
        return Unexpected(
            ERROR(fmt::format("unhandled extended opcode: {}", eop)));
    }
}
