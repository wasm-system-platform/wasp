#pragma once

#include <memory>

#include "grammar/instructions.hpp"
#include "grammar/types.hpp"
#include "grammar/values.hpp"
#include "util/error_handling.hpp"

namespace grammar {

/******************/
/* Custom Section */
/******************/

class CustomSection {
public:
    static constexpr uint8_t ID = 0x00;

    static Expected<CustomSection> parse(std::istream& in, uint32_t size);

    const std::string& getName() const { return name_; }
    const std::vector<uint8_t>& getBytes() const { return bytes_; }

    std::string toString() const;

private:
    Name name_;
    std::vector<uint8_t> bytes_;

    CustomSection(const Name& name, std::vector<uint8_t>&& bytes);
};

/****************/
/* Type Section */
/****************/

class TypeSection {
public:
    static constexpr uint8_t ID = 0x01;

    TypeSection() {}

    static Expected<TypeSection> parse(std::istream& in);

    Expected<FunctionType> getFunctionType(uint32_t idx) const;
    const std::vector<FunctionType>& getFunctionTypes() const {
        return func_types_;
    }

    std::string toString() const;

private:
    std::vector<FunctionType> func_types_;

    TypeSection(std::vector<FunctionType>&& func_types);
};

/******************/
/* Import Section */
/******************/

class ImportBase;
using Import = std::shared_ptr<ImportBase>;

class ImportBase {
public:
    static Expected<Import> parse(std::istream& in);

    std::string toString() const;

    template <class Derived> bool is() const {
        return desc_id_ == Derived::DESC_ID;
    }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }

protected:
    Name module_;
    Name name_;

    ImportBase(uint8_t desc_id, const Name& module, const Name& name);

private:
    uint8_t desc_id_;
};

class FunctionImport : public ImportBase {
public:
    static constexpr uint8_t DESC_ID = 0x00;

    FunctionImport(const Name& module, const Name& name, uint32_t type_idx);

    const Name& getModule() const { return ImportBase::module_; }
    const Name& getName() const { return ImportBase::name_; }
    uint32_t getTypeIdx() const { return type_idx_; }

    std::string toString() const;

private:
    uint32_t type_idx_;
};

class MemoryImport : public ImportBase {
public:
    static constexpr uint8_t DESC_ID = 0x02;

    MemoryImport(const Name& module, const Name& name,
                 const MemoryType& mem_type);

    const MemoryType& getMemoryType() const { return mem_type_; }

    std::string toString() const;

private:
    MemoryType mem_type_;
};

class ImportSection {
public:
    static constexpr uint8_t ID = 0x02;

    ImportSection() {}

    static Expected<ImportSection> parse(std::istream& in);

    const std::vector<FunctionImport>& getFunctionImports() const {
        return func_imports_;
    }

    const std::vector<MemoryImport>& getMemoryImports() const {
        return mem_imports_;
    }

    std::string toString() const;

private:
    std::vector<FunctionImport> func_imports_;
    std::vector<MemoryImport> mem_imports_;

    ImportSection(std::vector<FunctionImport>&& func_imports,
                  std::vector<MemoryImport>&& mem_imports);
};

/********************/
/* Function Section */
/********************/

class FunctionSection {
public:
    static constexpr uint8_t ID = 0x03;

    static Expected<FunctionSection> parse(std::istream& in);

    const std::vector<uint32_t>& getTypes() const { return type_indices_; }

    std::string toString() const;

private:
    std::vector<uint32_t> type_indices_;

    FunctionSection(std::vector<uint32_t>&& type_indices);
};

/*****************/
/* Table Section */
/*****************/

class TableSection {
public:
    static constexpr uint8_t ID = 0x04;

    static Expected<TableSection> parse(std::istream& in);

    std::string toString() const;

private:
    std::vector<TableType> tables_;

    TableSection(std::vector<TableType>&& tables);
};

/******************/
/* Global Section */
/******************/

class Global : public GlobalType, public Expression {
public:
    static Expected<Global> parse(std::istream& in);

    std::string toString() const;

private:
    Global(const GlobalType& global_type, const Expression& expression);
};

class GlobalSection {
public:
    GlobalSection() = default;

    static constexpr uint8_t ID = 0x06;

    static Expected<GlobalSection> parse(std::istream& in);

    const std::vector<Global>& getGlobals() const { return globals_; }

    std::string toString() const;

private:
    std::vector<Global> globals_;

    GlobalSection(std::vector<Global>&& globals);
};

/******************/
/* Export Section */
/******************/

class ExportBase;
using Export = std::shared_ptr<ExportBase>;

class ExportBase : public Name {
public:
    static Expected<Export> parse(std::istream& in);

    const std::string& getName() const { return *this; }
    uint32_t getIdx() const { return idx_; }

    template <class Derived> bool is() const { return type_ == Derived::TYPE; }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }

protected:
    enum class DescriptorType {
        func = 0x00,
        table = 0x01,
        mem = 0x02,
        global = 0x03,
    };

    uint32_t idx_;

    ExportBase(const Name& name, DescriptorType type, uint32_t idx)
        : Name(name), idx_(idx), type_(type) {}

private:
    DescriptorType type_;
};

class FunctionExport : public ExportBase {
public:
    FunctionExport(const Name& name, uint32_t idx)
        : ExportBase(name, DescriptorType::func, idx) {}

    std::string toString() const;

private:
    static constexpr DescriptorType TYPE = DescriptorType::func;

    friend class ExportBase;
};

class ExportSection {
public:
    static constexpr uint8_t ID = 0x07;

    static Expected<ExportSection> parse(std::istream& in);

    const std::vector<FunctionExport>& getFunctionExports() const {
        return func_exports_;
    }

    std::string toString() const;

private:
    std::vector<FunctionExport> func_exports_;

    ExportSection(std::vector<FunctionExport>&& func_exports)
        : func_exports_(std::move(func_exports)) {}
};

/*****************/
/* Start Section */
/*****************/

class StartSection {
public:
    static constexpr uint8_t ID = 0x08;

    static Expected<StartSection> parse(std::istream& in);

    uint32_t getFunctionIdx() const { return func_idx_; }

    std::string toString() const;

private:
    uint32_t func_idx_;

    StartSection(uint32_t func_idx) : func_idx_(func_idx) {}
};

/*******************/
/* Element Section */
/*******************/

class ElementSegment {
public:
    static Expected<ElementSegment> parse(std::istream& in);

    std::string toString() const;

    const Expression& getOffset() const { return offset_; }
    const std::vector<uint32_t>& getFunctionRefs() const { return func_refs_; }

private:
    Expression offset_;
    std::vector<uint32_t> func_refs_;

    ElementSegment(const Expression& offset, std::vector<uint32_t>&& func_refs)
        : offset_(offset), func_refs_(std::move(func_refs)) {}
};

class ElemenSection {
public:
    static constexpr uint8_t ID = 0x09;

    ElemenSection() = default;

    static Expected<ElemenSection> parse(std::istream& in);

    std::string toString() const;

    const std::vector<ElementSegment>& getSegments() const { return segments_; }

private:
    std::vector<ElementSegment> segments_;

    ElemenSection(std::vector<ElementSegment>&& segments)
        : segments_(std::move(segments)) {}
};

/****************/
/* Code Section */
/****************/

class Function {
public:
    static Expected<Function> parse(std::istream& in, size_t code_start);

    const std::vector<ValueType>& getLocals() const { return locals_; }
    const Expression& getBody() const { return body_; }

    std::string toString() const;

private:
    std::vector<ValueType> locals_;
    Expression body_;

    Function(std::vector<ValueType>&& locals, const Expression& body)
        : locals_(std::move(locals)), body_(body) {}
};

class CodeSection {
public:
    static constexpr uint8_t ID = 10;

    static Expected<CodeSection> parse(std::istream& in, size_t code_start);

    const std::vector<Function>& getFunctions() const { return funcs_; }
    size_t getCodeStart() const { return code_start_; }

    std::string toString() const;

private:
    std::vector<Function> funcs_;
    size_t code_start_;

    CodeSection(std::vector<Function>&& funcs, size_t code_start)
        : funcs_(std::move(funcs)), code_start_(code_start) {}
};

/****************/
/* Data Section */
/****************/

class Segment {
public:
    static Expected<Segment> parse(std::istream& in);

    const std::vector<uint8_t>& getBytes() const { return bytes_; }

    bool isActive() const { return offset_opt_.has_value(); }

    const Expression& getOffset() const { return offset_opt_.value(); }

    std::string toString() const;

private:
    std::vector<uint8_t> bytes_;
    std::optional<Expression> offset_opt_;

    Segment(std::vector<uint8_t>&& bytes, std::optional<Expression>&& offset)
        : bytes_(std::move(bytes)), offset_opt_(std::move(offset)) {}
};

class DataSection {
public:
    static constexpr uint8_t ID = 11;

    DataSection() {}

    static Expected<DataSection> parse(std::istream& in);

    const std::vector<Segment>& getSegments() const { return segments_; }

    std::string toString() const;

private:
    std::vector<Segment> segments_;

    DataSection(std::vector<Segment>&& segments)
        : segments_(std::move(segments)) {}
};

/**********************/
/* Data Count Section */
/**********************/

class DataCountSection {
public:
    static constexpr uint8_t ID = 12;

    static Expected<DataCountSection> parse(std::istream& in);

    std::string toString() const;

private:
    uint32_t data_count_;

    DataCountSection(uint32_t data_count) : data_count_(data_count) {}
};

} // namespace grammar
