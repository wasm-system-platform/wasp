#pragma once

#include <optional>

#include "grammar/dwarf.hpp"
#include "sections.hpp"

namespace grammar {

class Module {
public:
    static Expected<Module> parse(std::istream& in);

    const bool hasFunctionSection() const { return func_section_.has_value(); }
    const bool hasExportSection() const { return export_section_.has_value(); }
    const bool hasCodeSection() const { return code_section_.has_value(); }
    const bool hasStartSection() const { return start_section_.has_value(); }

    const TypeSection& getTypeSection() const { return type_section_; }
    const ImportSection& getImportSection() const { return import_section_; }
    const GlobalSection& getGlobalSection() const { return global_section_; }
    const FunctionSection& getFunctionSection() const { return *func_section_; }
    const ExportSection& getExportSection() const { return *export_section_; }
    const StartSection& getStartSection() const { return *start_section_; }
    const ElemenSection& getElementSection() const { return element_section_; }
    const CodeSection& getCodeSection() const { return *code_section_; }
    const DataSection& getDataSection() const { return data_section_; };
    const DebugLineSection& getDebugLineSection() const {
        return debug_line_section_;
    }

private:
    TypeSection type_section_;
    ImportSection import_section_;
    std::optional<FunctionSection> func_section_;
    std::optional<TableSection> table_section_;
    GlobalSection global_section_;
    std::optional<ExportSection> export_section_;
    std::optional<StartSection> start_section_;
    ElemenSection element_section_;
    std::optional<CodeSection> code_section_;
    DataSection data_section_;
    DebugLineSection debug_line_section_;

    Module(TypeSection&& type_section, ImportSection&& import_section,
           std::optional<FunctionSection>&& function_section,
           std::optional<TableSection>&& table_section,
           GlobalSection&& global_section,
           std::optional<ExportSection>&& export_section,
           std::optional<StartSection>&& start_section,
           ElemenSection&& element_section,
           std::optional<CodeSection>&& code_section, DataSection data_section,
           DebugLineSection&& debug_line_section)
        : type_section_(std::move(type_section)),
          import_section_(std::move(import_section)),
          func_section_(std::move(function_section)),
          table_section_(std::move(table_section)),
          global_section_(std::move(global_section)),
          export_section_(std::move(export_section)),
          start_section_(std::move(start_section)),
          element_section_(std::move(element_section)),
          code_section_(std::move(code_section)),
          data_section_(std::move(data_section)),
          debug_line_section_(std::move(debug_line_section)) {}
};

} // namespace grammar
