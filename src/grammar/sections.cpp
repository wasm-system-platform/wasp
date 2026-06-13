#include <fmt/format.h>
#include <fmt/ranges.h>
#include <fstream>

#include "grammar/sections.hpp"
#include "grammar/types.hpp"
#include "grammar/values.hpp"

namespace grammar {

/******************/
/* Custom Section */
/******************/

Expected<CustomSection> CustomSection::parse(std::istream& in, uint32_t size) {
    size_t start = static_cast<size_t>(in.tellg());

    Expected<Name> name_exp = Name::parse(in);
    if (!name_exp)
        return Unexpected(PROPAGATE(name_exp));

    size_t offset = static_cast<size_t>(in.tellg()) - start;
    if (offset > size)
        return Unexpected(ERROR("custom section has an invalid size"));

    std::vector<uint8_t> bytes(size - offset);
    for (size_t i = 0; i < size - offset; i++) {
        Expected<Byte> b_exp = Byte::parse(in);
        if (!b_exp)
            return Unexpected(PROPAGATE(b_exp));

        bytes[i] = *b_exp;
    }

    return CustomSection(*name_exp, std::move(bytes));
}

CustomSection::CustomSection(const Name& name, std::vector<uint8_t>&& bytes)
    : name_(name), bytes_(std::move(bytes)) {}

/****************/
/* Type Section */
/****************/

Expected<TypeSection> TypeSection::parse(std::istream& in) {
    Expected<U32> len_exp = U32::parse(in);
    if (!len_exp)
        return Unexpected(PROPAGATE(len_exp));

    uint32_t len = static_cast<uint32_t>(*len_exp);
    std::vector<FunctionType> func_types;
    func_types.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<FunctionType> ft_exp = FunctionType::parse(in);
        if (!ft_exp)
            return Unexpected(PROPAGATE(ft_exp));

        func_types.push_back(*ft_exp);
    }

    return TypeSection(std::move(func_types));
}

Expected<FunctionType> TypeSection::getFunctionType(uint32_t idx) const {
    if (idx > func_types_.size())
        return Unexpected(ERROR("function type index is out of bounds"));

    return func_types_[idx];
}

std::string TypeSection::toString() const {
    std::vector<std::string> fmt_fts;
    fmt_fts.reserve(func_types_.size());
    for (size_t i = 0; i < func_types_.size(); i++)
        fmt_fts.push_back(
            fmt::format(" - type[{}] {}", i, func_types_[i].toString()));

    return fmt::format("Type[{}]:\n{}", func_types_.size(),
                       fmt::join(fmt_fts, "\n"));
}

TypeSection::TypeSection(std::vector<FunctionType>&& func_types)
    : func_types_(std::move(func_types)) {}

/******************/
/* Import Section */
/******************/

Expected<Import> ImportBase::parse(std::istream& in) {
    Expected<Name> module_res = Name::parse(in);
    if (!module_res)
        return Unexpected(PROPAGATE(module_res));
    const Name& module = *module_res;

    Expected<Name> name_res = Name::parse(in);
    if (!name_res)
        return Unexpected(PROPAGATE(name_res));
    const Name& name = *name_res;

    Expected<U32> desc_id_res = U32::parse(in);
    if (!desc_id_res)
        return Unexpected(PROPAGATE(desc_id_res));
    uint32_t desc_id = *desc_id_res;

    switch (desc_id) {
    case FunctionImport::DESC_ID: {
        Expected<U32> type_idx_res = U32::parse(in);
        if (!type_idx_res)
            return Unexpected(PROPAGATE(type_idx_res));
        uint32_t type_idx = *type_idx_res;

        return std::make_shared<FunctionImport>(module, name, type_idx);
    }
    case MemoryImport::DESC_ID: {
        Expected<MemoryType> mem_type_res = MemoryType::parse(in);
        if (!mem_type_res)
            return Unexpected(PROPAGATE(mem_type_res));
        const MemoryType& mem_type = *mem_type_res;

        return std::make_shared<MemoryImport>(module, name, mem_type);
    }
    default:
        return Unexpected(
            ERROR(fmt::format("unknown descriptor id: {}", desc_id)));
    }
};

ImportBase::ImportBase(uint8_t desc_id, const Name& module, const Name& name)
    : module_(module), name_(name), desc_id_(desc_id) {}

std::string ImportBase::toString() const {
    return fmt::format("{}.{}", static_cast<const std::string&>(module_),
                       static_cast<const std::string&>(name_));
}

FunctionImport::FunctionImport(const Name& module, const Name& name,
                               uint32_t type_idx)
    : ImportBase(FunctionImport::DESC_ID, module, name), type_idx_(type_idx) {}

std::string FunctionImport::toString() const {
    return fmt::format("sig={} <- {}", type_idx_, ImportBase::toString());
}

MemoryImport::MemoryImport(const Name& module, const Name& name,
                           const MemoryType& mem_type)
    : ImportBase(MemoryImport::DESC_ID, module, name), mem_type_(mem_type) {}

std::string MemoryImport::toString() const {
    return fmt::format("pages: initial={} max={} {} <- {}",
                       mem_type_.getInitial(), mem_type_.getMax(),
                       (mem_type_.isShared() ? "shared" : ""),
                       ImportBase::toString());
}

Expected<ImportSection> ImportSection::parse(std::istream& in) {
    std::vector<FunctionImport> func_imports;
    std::vector<MemoryImport> mem_imports;

    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    for (uint32_t i = 0; i < len; i++) {
        Expected<Import> import_res = ImportBase::parse(in);
        if (!import_res)
            return Unexpected(PROPAGATE(import_res));

        const Import& import = *import_res;
        if (import->is<FunctionImport>()) {
            func_imports.push_back(import->as<FunctionImport>());
        } else if (import->is<MemoryImport>()) {
            mem_imports.push_back(import->as<MemoryImport>());
        } else {
            Unexpected(ERROR(fmt::format("unexpected memory import type")));
        }
    }

    return ImportSection(std::move(func_imports), std::move(mem_imports));
}

std::string ImportSection::toString() const {
    std::vector<std::string> fmt_imports;
    fmt_imports.reserve(func_imports_.size() + mem_imports_.size());

    for (size_t i = 0; i < func_imports_.size(); i++) {
        fmt_imports.push_back(
            fmt::format(" - func[{}] {}", i, func_imports_[i].toString()));
    }

    for (size_t i = 0; i < mem_imports_.size(); i++) {
        fmt_imports.push_back(
            fmt::format(" - memory[{}] {}", i, mem_imports_[i].toString()));
    }

    return fmt::format("Import[{}]:\n{}", fmt_imports.size(),
                       fmt::join(fmt_imports, "\n"));
}

ImportSection::ImportSection(std::vector<FunctionImport>&& func_imports,
                             std::vector<MemoryImport>&& mem_imports)
    : func_imports_(std::move(func_imports)),
      mem_imports_(std::move(mem_imports)) {}

/********************/
/* Function Section */
/********************/

Expected<FunctionSection> FunctionSection::parse(std::istream& in) {
    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    std::vector<uint32_t> type_indices;
    type_indices.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<U32> type_idx_res = U32::parse(in);
        if (!type_idx_res)
            return Unexpected(PROPAGATE(type_idx_res));
        uint32_t type_idx = static_cast<uint32_t>(*type_idx_res);

        type_indices.push_back(type_idx);
    }

    return FunctionSection(std::move(type_indices));
}

std::string FunctionSection::toString() const {
    std::vector<std::string> fmt_funcs;
    fmt_funcs.reserve(type_indices_.size());

    for (size_t i = 0; i < type_indices_.size(); i++) {
        fmt_funcs.push_back(
            fmt::format(" - func[{}] sig={}", i, type_indices_[i]));
    }

    return fmt::format("Function[{}]:\n{}", fmt_funcs.size(),
                       fmt::join(fmt_funcs, "\n"));
}

FunctionSection::FunctionSection(std::vector<uint32_t>&& type_indices)
    : type_indices_(std::move(type_indices)) {}

/*****************/
/* Table Section */
/*****************/

Expected<TableSection> TableSection::parse(std::istream& in) {
    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    std::vector<TableType> tables;
    tables.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<TableType> table_res = TableType::parse(in);
        if (!table_res)
            return Unexpected(PROPAGATE(table_res));
        tables.push_back(*table_res);
    }

    return TableSection(std::move(tables));
}

std::string TableSection::toString() const {
    std::vector<std::string> fmt_tables;
    fmt_tables.reserve(tables_.size());

    for (size_t i = 0; i < tables_.size(); i++) {
        fmt_tables.push_back(
            fmt::format(" - tables[{}] {}", i, tables_[i].toString()));
    }

    return fmt::format("Table[{}]:\n{}", fmt_tables.size(),
                       fmt::join(fmt_tables, "\n"));
}

TableSection::TableSection(std::vector<TableType>&& tables)
    : tables_(std::move(tables)) {}

/******************/
/* Global Section */
/******************/

Expected<Global> Global::parse(std::istream& in) {
    Expected<GlobalType> global_type_res = GlobalType::parse(in);
    if (!global_type_res)
        return Unexpected(PROPAGATE(global_type_res));

    Expected<Expression> expr_res = Expression::parse(in, UINT32_MAX);
    if (!expr_res)
        return Unexpected(PROPAGATE(expr_res));

    return Global(std::move(*global_type_res), std::move(*expr_res));
}

std::string Global::toString() const {
    return fmt::format("{} - init {}", GlobalType::toString(),
                       Expression::toString());
}

Global::Global(const GlobalType& global_type, const Expression& expression)
    : GlobalType(global_type), Expression(expression) {}

Expected<GlobalSection> GlobalSection::parse(std::istream& in) {
    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    std::vector<Global> globals;
    globals.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<Global> global_res = Global::parse(in);
        if (!global_res)
            return Unexpected(PROPAGATE(global_res));
        globals.push_back(*global_res);
    }

    return GlobalSection(std::move(globals));
}

std::string GlobalSection::toString() const {
    std::vector<std::string> fmt_globals;
    fmt_globals.reserve(globals_.size());

    for (size_t i = 0; i < globals_.size(); i++) {
        fmt_globals.push_back(
            fmt::format(" - global[{}] {}", i, globals_[i].toString()));
    }

    return fmt::format("Global[{}]:\n{}", fmt_globals.size(),
                       fmt::join(fmt_globals, "\n"));
}

GlobalSection::GlobalSection(std::vector<Global>&& globals)
    : globals_(std::move(globals)) {}

/******************/
/* Export Section */
/******************/

Expected<Export> ExportBase::parse(std::istream& in) {
    Expected<Name> name_res = Name::parse(in);
    if (!name_res)
        return Unexpected(PROPAGATE(name_res));
    const Name& name = *name_res;

    Expected<Byte> descriptor_type_res = Byte::parse(in);
    if (!descriptor_type_res)
        return Unexpected(PROPAGATE(descriptor_type_res));
    uint8_t descriptor_type = *descriptor_type_res;

    Expected<U32> idx_res = U32::parse(in);
    if (!idx_res)
        return Unexpected(PROPAGATE(idx_res));
    uint32_t idx = *idx_res;

    switch (descriptor_type) {
    case static_cast<uint8_t>(DescriptorType::func):
        return std::make_shared<FunctionExport>(name, idx);
    default:
        return Unexpected(ERROR(
            fmt::format("unknown descriptor type: 0x{:X}", descriptor_type)));
    }
}

std::string FunctionExport::toString() const {
    return fmt::format("func={} -> {}", idx_, static_cast<std::string>(*this));
}

Expected<ExportSection> ExportSection::parse(std::istream& in) {
    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    std::vector<FunctionExport> func_exports;
    func_exports.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<Export> export_res = ExportBase::parse(in);
        if (!export_res)
            return Unexpected(PROPAGATE(export_res));
        const Export& exp = *export_res;

        if (exp->is<FunctionExport>()) {
            func_exports.push_back(exp->as<FunctionExport>());
        } else {
            return Unexpected(ERROR("invalid export object"));
        }
    }

    return ExportSection(std::move(func_exports));
}

std::string ExportSection::toString() const {
    std::vector<std::string> fmt_exports;
    fmt_exports.reserve(func_exports_.size());

    for (size_t i = 0; i < func_exports_.size(); i++) {
        fmt_exports.push_back(
            fmt::format(" - func[{}] {}", i, func_exports_[i].toString()));
    }

    return fmt::format("Export[{}]:\n{}", fmt_exports.size(),
                       fmt::join(fmt_exports, "\n"));
}

/*****************/
/* Start Section */
/*****************/

Expected<StartSection> StartSection::parse(std::istream& in) {
    Expected<U32> func_idx_res = U32::parse(in);
    if (!func_idx_res)
        return Unexpected(PROPAGATE(func_idx_res));
    return StartSection(StartSection(*func_idx_res));
}

std::string StartSection::toString() const {
    return fmt::format("Start:\n - start func={}", func_idx_);
}

/*******************/
/* Element Section */
/*******************/

Expected<ElementSegment> ElementSegment::parse(std::istream& in) {
    Expected<U32> bitfield_res = U32::parse(in);
    if (!bitfield_res)
        return Unexpected(PROPAGATE(bitfield_res));
    uint32_t bitfield = *bitfield_res;

    if (bitfield != 0)
        return Unexpected(ERROR("unsupported element format"));

    Expected<Expression> offset_res = Expression::parse(in, UINT32_MAX);
    if (!offset_res)
        return Unexpected(PROPAGATE(offset_res));

    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    std::vector<uint32_t> func_refs;
    func_refs.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<U32> func_idx_res = U32::parse(in);
        if (!func_idx_res)
            return Unexpected(PROPAGATE(func_idx_res));
        func_refs.push_back(*func_idx_res);
    }

    return ElementSegment(*offset_res, std::move(func_refs));
}

std::string ElementSegment::toString() const {
    std::vector<std::string> fmt_func_indices;
    fmt_func_indices.reserve(func_refs_.size());

    for (size_t i = 0; i < func_refs_.size(); i++) {
        fmt_func_indices.push_back(
            fmt::format("   - func[{}] func={}", i, func_refs_[i]));
    }

    return fmt::format("count={} - init {}\n{}", offset_.toString(),
                       fmt_func_indices.size(),
                       fmt::join(fmt_func_indices, "\n"));
}

Expected<ElemenSection> ElemenSection::parse(std::istream& in) {
    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    std::vector<ElementSegment> elements;
    elements.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<ElementSegment> element_res = ElementSegment::parse(in);
        if (!element_res)
            return Unexpected(PROPAGATE(element_res));
        elements.push_back(*element_res);
    }

    return ElemenSection(std::move(elements));
}

std::string ElemenSection::toString() const {
    std::vector<std::string> fmt_elements;
    fmt_elements.reserve(segments_.size());

    for (size_t i = 0; i < segments_.size(); i++) {
        fmt_elements.push_back(
            fmt::format(" - elem[{}] {}", i, segments_[i].toString()));
    }

    return fmt::format("Elem[{}]:\n{}", fmt_elements.size(),
                       fmt::join(fmt_elements, "\n"));
}

/****************/
/* Code Section */
/****************/

Expected<Function> Function::parse(std::istream& in, size_t code_start) {
    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    std::vector<ValueType> locals;
    locals.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<U32> count_res = U32::parse(in);
        if (!count_res)
            return Unexpected(PROPAGATE(count_res));
        uint32_t count = *count_res;

        Expected<ValueType> local_res = ValueType::parse(in);
        if (!local_res)
            return Unexpected(PROPAGATE(local_res));
        const ValueType& local = *local_res;

        locals.reserve(locals.size() + count);
        for (uint32_t j = 0; j < count; j++)
            locals.push_back(local);
    }

    Expected<Expression> body_res = Expression::parse(in, code_start);
    if (!body_res)
        return Unexpected(PROPAGATE(body_res));

    return Function(std::move(locals), *body_res);
}

std::string Function::toString() const {
    std::vector<std::string> fmt_locals;
    fmt_locals.reserve(locals_.size());

    for (size_t i = 0; i < locals_.size(); i++)
        fmt_locals.push_back(locals_[i].toString());

    return fmt::format("locals=({})\n{}", fmt::join(fmt_locals, ", "),
                       body_.toString());
}

Expected<CodeSection> CodeSection::parse(std::istream& in, size_t code_start) {
    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    std::vector<Function> funcs;
    funcs.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<U32> size_res = U32::parse(in);
        if (!size_res)
            return Unexpected(PROPAGATE(size_res));

        Expected<Function> func_res = Function::parse(in, code_start);
        if (!func_res)
            return Unexpected(PROPAGATE(func_res));
        funcs.push_back(*func_res);
    }

    return CodeSection(std::move(funcs), code_start);
}

std::string CodeSection::toString() const {
    std::vector<std::string> fmt_funcs;
    fmt_funcs.reserve(funcs_.size());

    for (size_t i = 0; i < funcs_.size(); i++) {
        fmt_funcs.push_back(
            fmt::format(" - func[{}] {}", i, funcs_[i].toString()));
    }

    return fmt::format("Code[{}]:\n{}", fmt_funcs.size(),
                       fmt::join(fmt_funcs, "\n"));
}

/****************/
/* Data Section */
/****************/

Expected<Segment> Segment::parse(std::istream& in) {
    Expected<Byte> b_res = Byte::parse(in);
    if (!b_res)
        return Unexpected(PROPAGATE(b_res));

    uint8_t b = *b_res;

    std::optional<Expression> offset_opt;
    if (b == 0x00) {
        Expected<Expression> offset_exp = Expression::parse(in, UINT32_MAX);
        if (!offset_exp)
            return Unexpected(PROPAGATE(offset_exp));

        offset_opt = *offset_exp;
    } else if (b != 0x01) {
        return Unexpected(
            ERROR("multi memory data segments currently not supported"));
    }

    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    std::vector<uint8_t> bytes;
    bytes.resize(len);

    if (!in.read(reinterpret_cast<char*>(bytes.data()), static_cast<uint32_t>(bytes.size()))) {
        return Unexpected(ERROR("unexpected end of file"));
    }

    return Segment(std::move(bytes), std::move(offset_opt));
}

std::string Segment::toString() const {
    fmt::memory_buffer buf;

    for (size_t i = 0; i < bytes_.size(); ++i) {
        if (i % 16 == 0) {
            fmt::format_to(std::back_inserter(buf), "\n   - {:08X}: ", i);
        }
        fmt::format_to(std::back_inserter(buf), "{:02X} ", bytes_[i]);
    }

    return fmt::to_string(buf);
}

Expected<DataSection> DataSection::parse(std::istream& in) {
    Expected<U32> len_res = U32::parse(in);
    if (!len_res)
        return Unexpected(PROPAGATE(len_res));
    uint32_t len = static_cast<uint32_t>(*len_res);

    std::vector<Segment> segments;
    segments.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<Segment> data_res = Segment::parse(in);
        if (!data_res)
            return Unexpected(PROPAGATE(data_res));

        segments.push_back(*data_res);
    }

    return DataSection(std::move(segments));
}

std::string DataSection::toString() const {
    std::vector<std::string> fmt_segments;
    fmt_segments.reserve(segments_.size());

    for (size_t i = 0; i < segments_.size(); i++) {
        fmt_segments.push_back(fmt::format(" - segment[{}] size={} {}", i,
                                           segments_.size(),
                                           segments_[i].toString()));
    }

    return fmt::format("Data[{}]:\n{}", fmt_segments.size(),
                       fmt::join(fmt_segments, "\n"));
}

/**********************/
/* Data Count Section */
/**********************/

Expected<DataCountSection> DataCountSection::parse(std::istream& in) {
    Expected<U32> data_count_res = U32::parse(in);
    if (!data_count_res)
        return Unexpected(PROPAGATE(data_count_res));
    uint32_t data_count = *data_count_res;

    return DataCountSection(data_count);
}

std::string DataCountSection::toString() const {
    return fmt::format("DataCount:\n - count={}", data_count_);
}

} // namespace grammar
