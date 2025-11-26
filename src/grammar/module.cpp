#include <array>
#include <iostream>
#include <sstream>

#include <fmt/format.h>

#include "grammar/dwarf.hpp"
#include "grammar/module.hpp"

namespace grammar {

Expected<Module> Module::parse(std::istream& in) {
    if (!in) {
        return Unexpected(ERROR("broken file stream"));
    }

    constexpr std::array<unsigned char, 4> expected_magic = {0x00, 0x61, 0x73,
                                                             0x6D};
    constexpr std::array<unsigned char, 4> expected_version = {0x01, 0x00, 0x00,
                                                               0x00};

    std::array<unsigned char, 4> magic{};
    std::array<unsigned char, 4> version{};

    if (!in.read(reinterpret_cast<char*>(magic.data()), magic.size()) ||
        magic != expected_magic) {
        return Unexpected(ERROR("invalid or missing magic bytes"));
    }

    if (!in.read(reinterpret_cast<char*>(version.data()), version.size()) ||
        version != expected_version) {
        return Unexpected(ERROR("invalid or missing magic bytes"));
    }

    std::optional<TypeSection> type_section;
    std::optional<ImportSection> import_section;
    std::optional<FunctionSection> func_section;
    std::optional<TableSection> table_section;
    std::optional<GlobalSection> global_section;
    std::optional<ExportSection> export_section;
    std::optional<StartSection> start_section;
    ElemenSection element_section;
    std::optional<CodeSection> code_section;
    std::optional<DataSection> data_section;
    DebugLineSection debug_line_section;

    std::vector<CustomSection> custom_sections;

    while (true) {
        size_t module_start = static_cast<size_t>(in.tellg());

        Expected<Byte> section_id_exp = Byte::parse(in);
        if (!section_id_exp)
            return Unexpected(PROPAGATE(section_id_exp));
        uint8_t section_id = *section_id_exp;

        Expected<U32> size_res = U32::parse(in);
        if (!size_res)
            return Unexpected(PROPAGATE(size_res));
        uint32_t size = *size_res;

        switch (section_id) {
        case CustomSection::ID: {
            Expected<CustomSection> custom_section_res =
                CustomSection::parse(in, size);
            if (!custom_section_res)
                return Unexpected(PROPAGATE(custom_section_res));

            custom_sections.emplace_back(std::move(*custom_section_res));
            break;
        }
        case TypeSection::ID: {
            Expected<TypeSection> type_section_res = TypeSection::parse(in);
            if (!type_section_res)
                return Unexpected(PROPAGATE(type_section_res));
            type_section = *type_section_res;
            break;
        }
        case ImportSection::ID: {
            Expected<ImportSection> import_section_res =
                ImportSection::parse(in);
            if (!import_section_res)
                return Unexpected(PROPAGATE(import_section_res));
            import_section = *import_section_res;
            break;
        }
        case FunctionSection::ID: {
            Expected<FunctionSection> func_section_res =
                FunctionSection::parse(in);
            if (!func_section_res)
                return Unexpected(PROPAGATE(func_section_res));
            func_section = *func_section_res;
            break;
        }
        case TableSection::ID: {
            Expected<TableSection> table_section_res = TableSection::parse(in);
            if (!table_section_res)
                return Unexpected(PROPAGATE(table_section_res));
            table_section = *table_section_res;
            break;
        }
        case GlobalSection::ID: {
            Expected<GlobalSection> global_section_res =
                GlobalSection::parse(in);
            if (!global_section_res)
                return Unexpected(PROPAGATE(global_section_res));
            global_section = *global_section_res;
            break;
        }
        case ExportSection::ID: {
            Expected<ExportSection> export_section_res =
                ExportSection::parse(in);
            if (!export_section_res)
                return Unexpected(PROPAGATE(export_section_res));
            export_section = *export_section_res;
            break;
        }
        case StartSection::ID: {
            Expected<StartSection> start_section_res = StartSection::parse(in);
            if (!start_section_res)
                return Unexpected(PROPAGATE(start_section_res));
            start_section = *start_section_res;
            break;
        }
        case ElemenSection::ID: {
            Expected<ElemenSection> element_section_res =
                ElemenSection::parse(in);
            if (!element_section_res)
                return Unexpected(PROPAGATE(element_section_res));
            element_section = std::move(*element_section_res);
            break;
        }
        case CodeSection::ID: {
            Expected<CodeSection> code_section_res =
                CodeSection::parse(in, module_start);
            if (!code_section_res)
                return Unexpected(PROPAGATE(code_section_res));
            code_section = *code_section_res;
            break;
        }
        case DataSection::ID: {
            Expected<DataSection> data_section_res = DataSection::parse(in);
            if (!data_section_res)
                return Unexpected(PROPAGATE(data_section_res));
            data_section = *data_section_res;
            break;
        }
        case DataCountSection::ID: {
            Expected<DataCountSection> data_count_section_res =
                DataCountSection::parse(in);
            if (!data_count_section_res)
                return Unexpected(PROPAGATE(data_count_section_res));
            // ignore for now
            break;
        }
        default:
            return Unexpected(
                ERROR(fmt::format("unknown section id: {}", section_id)));
        }

        if (in.peek() == std::char_traits<char>::eof())
            break;
    }

    for (const auto& custom_section : custom_sections) {
        if (custom_section.getName() != ".debug_line")
            continue;

        const std::vector<uint8_t>& custom_bytes = custom_section.getBytes();
        std::string custom_string(
            reinterpret_cast<const char*>(custom_bytes.data()),
            custom_bytes.size());
        std::stringstream custom_stream(custom_string);

        Expected<DebugLineSection> debug_line_section_exp =
            DebugLineSection::parse(custom_stream);
        if (!debug_line_section_exp) {
            fmt::println("[warning] failed to parse debug line section: {}",
                         debug_line_section_exp.error().toString());
            continue;
        }

        debug_line_section = std::move(*debug_line_section_exp);
    }

    return Module(
        type_section.value_or(TypeSection()),
        import_section.value_or(ImportSection()), std::move(func_section),
        std::move(table_section), global_section.value_or(GlobalSection()),
        std::move(export_section), std::move(start_section),
        std::move(element_section), std::move(code_section),
        data_section.value_or(DataSection()), std::move(debug_line_section));
}

} // namespace grammar
