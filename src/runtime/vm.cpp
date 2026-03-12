#include <algorithm>
#include <fmt/format.h>

#include "runtime/optimization.hpp"
#include "runtime/vm.hpp"

namespace runtime {

/********************/
/* Global Instances */
/********************/

Expected<GlobalInstances>
GlobalInstances::create(const grammar::Module& module) {
    std::vector<Value> globals;

    const grammar::GlobalSection& global_section = module.getGlobalSection();
    for (const auto& global : global_section.getGlobals()) {
        std::vector<Operation> containers;

        Context dummy_context(0);
        std::vector<FunctionType> dummy_types;
        const Operation op = OperationBase::create(global.getInstructions(),
                                                   dummy_types, containers);

        Expected<Continuation> continuation_exp = op->eval(dummy_context);
        if (!continuation_exp)
            return Unexpected(PROPAGATE(continuation_exp));

        while (continuation_exp.value()) {
            continuation_exp = continuation_exp.value()->eval(dummy_context);
            if (!continuation_exp)
                return Unexpected(PROPAGATE(continuation_exp));
        }

        if (dummy_context.size() != 1)
            return Unexpected(ERROR("malformed constant expression"));

        globals.push_back(dummy_context.pop());
    }

    return GlobalInstances(std::move(globals));
}

/******************/
/* Data Instances */
/******************/

Expected<DataInstances> DataInstances::create(const grammar::Module& module) {
    const grammar::DataSection& data_section = module.getDataSection();
    const std::vector<grammar::Segment>& data_segments =
        data_section.getSegments();

    std::vector<std::vector<uint8_t>> segments;
    for (const grammar::Segment& segment : data_segments) {
        segments.push_back(segment.getBytes());
    }

    return DataInstances(std::move(segments));
}

Expected<GlobalState>
GlobalState::create(const grammar::Module& module,
                    const std::unordered_map<std::string, Function>& imports) {
    Expected<std::vector<Function>> funcs_exp =
        createFunctions(module, imports);
    if (!funcs_exp)
        return Unexpected(PROPAGATE(funcs_exp));

    Expected<std::vector<uint32_t>> indirect_funcs_exp =
        createIndirectFunctions(module);
    if (!indirect_funcs_exp)
        return Unexpected(PROPAGATE(indirect_funcs_exp));

    Expected<Memory> mem_exp = createMemory(module);
    if (!mem_exp)
        return Unexpected(PROPAGATE(mem_exp));

    Expected<GlobalInstances> global_exp = GlobalInstances::create(module);
    if (!global_exp)
        return Unexpected(PROPAGATE(global_exp));

    Expected<DataInstances> datas_exp = DataInstances::create(module);
    if (!datas_exp)
        return Unexpected(PROPAGATE(datas_exp));

    DebugInfoInstance debug_info = DebugInfoInstance::create(module);

    return GlobalState(std::move(*funcs_exp), std::move(*indirect_funcs_exp),
                       std::move(*mem_exp), *global_exp, *datas_exp,
                       std::move(debug_info));
}

/**************/
/* Debug Info */
/**************/

DebugInfoInstance DebugInfoInstance::create(const grammar::Module& module) {
    const grammar::CodeSection& code_section = module.getCodeSection();
    const DebugLineSection& debug_line_section = module.getDebugLineSection();

    const std::vector<DebugLineSection::Segment>& section_segments =
        debug_line_section.getSegments();

    std::vector<LineInfoSegment> segments;
    for (const auto& ss : section_segments) {
        LineInfoSegment s;
        s.start_addr = ss.start_addr;
        s.end_addr = ss.end_addr;
        s.src_file_idx = ss.src_file_idx;
        s.line = ss.line;

        segments.push_back(s);
    }

    std::sort(segments.begin(), segments.end(),
              [](const LineInfoSegment& a, const LineInfoSegment& b) {
                  return a.start_addr < b.start_addr;
              });

    return DebugInfoInstance(std::move(segments),
                             debug_line_section.getSourceFiles(),
                             code_section.getCodeStart());
}

std::string DebugInfoInstance::getFormattedLocation(size_t addr) const {
    std::string unknown_loc =
        addr == (uint32_t)-1 ? "<?>"
                             : fmt::format("<0x{:06x}>", addr + code_start_);

    if (line_info_segments_.empty())
        return unknown_loc;

    auto it =
        std::upper_bound(line_info_segments_.begin(), line_info_segments_.end(),
                         addr, [](size_t a, const LineInfoSegment& seg) {
                             return a < seg.start_addr;
                         });

    if (it == line_info_segments_.begin())
        return unknown_loc;

    --it;

    if (addr >= it->start_addr && addr < it->end_addr) {
        const std::string& file = src_files_[it->src_file_idx];
        return fmt::format("{}:{}", file, it->line);
    }

    return unknown_loc;
}

DebugInfoInstance::DebugInfoInstance(
    std::vector<LineInfoSegment>&& line_info_segments,
    const std::vector<std::string>& src_files, size_t code_start)
    : line_info_segments_(std::move(line_info_segments)), src_files_(src_files),
      code_start_(code_start) {}

Expected<std::vector<Function>> GlobalState::createFunctions(
    const grammar::Module& module,
    const std::unordered_map<std::string, Function>& imports) {
    const grammar::TypeSection& type_section = module.getTypeSection();
    const grammar::ImportSection& import_section = module.getImportSection();
    const grammar::FunctionSection& func_section = module.getFunctionSection();
    const grammar::CodeSection& code_section = module.getCodeSection();

    std::vector<Function> funcs;
    const std::vector<FunctionType>& func_types =
        type_section.getFunctionTypes();

    for (const auto& func : import_section.getFunctionImports()) {
        const std::string& module = func.getModule();
        const std::string& name = func.getName();
        std::string id = fmt::format("{}.{}", module, name);

        auto it = imports.find(id);
        if (it == imports.end())
            return Unexpected(
                ERROR(fmt::format("no function provided for {}", id)));

        const Function& import = it->second;

        Expected<FunctionType> type_exp =
            type_section.getFunctionType(func.getTypeIdx());
        if (!type_exp)
            return Unexpected(PROPAGATE(type_exp));

        if (!import.isCompatible(*type_exp))
            return Unexpected(ERROR(fmt::format(
                "function provided for {}.{} is not compatible with {}", module,
                name, type_exp->toString())));

        funcs.push_back(import);
    }

    const std::vector<uint32_t>& type_indices = func_section.getTypes();
    const std::vector<grammar::Function>& functions =
        code_section.getFunctions();

    if (type_indices.size() != functions.size())
        return Unexpected(ERROR(fmt::format(
            "size of type indices does not match size of functions ({} != {})",
            type_indices.size(), functions.size())));

    for (size_t i = 0; i < functions.size(); ++i) {
        Expected<FunctionType> type_exp =
            type_section.getFunctionType(type_indices[i]);
        if (!type_exp)
            return Unexpected(PROPAGATE(type_exp));

        funcs.push_back(Function::create(*type_exp, func_types, functions[i]));
    }

    printStats();
    return funcs;
}

Expected<std::vector<uint32_t>>
GlobalState::createIndirectFunctions(const grammar::Module& module) {
    const grammar::ElemenSection& element_section = module.getElementSection();

    const std::vector<grammar::ElementSegment>& segments =
        element_section.getSegments();
    if (segments.empty())
        return {};
    else if (segments.size() >= 2)
        return Unexpected(
            ERROR("multiple element segments are currently not supported"));

    const grammar::ElementSegment& segment = segments[0];

    Expected<int32_t> offset_exp = evaluateExpressionI32(segment.getOffset());
    if (!offset_exp.has_value())
        return Unexpected(PROPAGATE(offset_exp));
    uint32_t offset = static_cast<uint32_t>(*offset_exp);

    std::vector<uint32_t> indirect_funcs(offset, Function::NULL_IDX);
    for (uint32_t func_idx : segment.getFunctionRefs()) {
        indirect_funcs.push_back(func_idx);
    }

    return indirect_funcs;
}

Expected<Memory> GlobalState::createMemory(const grammar::Module& module) {
    const grammar::ImportSection& import_section = module.getImportSection();
    const grammar::DataSection& data_section = module.getDataSection();
    const std::vector<grammar::MemoryImport> mem_imports =
        import_section.getMemoryImports();

    if (mem_imports.empty())
        return Unexpected(ERROR("atleast one memory declaration is required"));

    if (mem_imports.size() > 1)
        return Unexpected(ERROR("multi memory is currently not supported"));

    const MemoryType& type = mem_imports[0].getMemoryType();
    Memory memory(type.getInitial(), type.getMax());

    /* copy active segments */
    const std::vector<grammar::Segment>& segments = data_section.getSegments();
    for (const auto& segment : segments) {
        if (!segment.isActive())
            continue;

        /* why is the offset an expression?! */
        const grammar::Expression& offset_expr = segment.getOffset();

        std::vector<Operation> targets;
        Context dummy_context(0);
        std::vector<FunctionType> dummy_types;
        const Operation offset_op = OperationBase::create(
            offset_expr.getInstructions(), dummy_types, targets);

        Expected<Continuation> continuation_exp =
            offset_op->eval(dummy_context);
        if (!continuation_exp)
            return Unexpected(PROPAGATE(continuation_exp));

        while (continuation_exp.value()) {
            continuation_exp = continuation_exp.value()->eval(dummy_context);
            if (!continuation_exp)
                return Unexpected(PROPAGATE(continuation_exp));
        }

        if (dummy_context.size() != 1)
            return Unexpected(ERROR("malformed constant expression"));

        uint32_t offset = dummy_context.pop().i32;

        if (!memory.store(offset, segment.getBytes()))
            return Unexpected(ERROR("failed to copy segment"));
    }

    return memory;
}

Expected<int32_t>
GlobalState::evaluateExpressionI32(const grammar::Expression& expression) {
    std::vector<FunctionType> dummy_types;
    std::vector<Operation> targets;
    Operation operation = OperationBase::create(expression.getInstructions(),
                                                dummy_types, targets);

    Context dummy_context(0);
    Expected<Continuation> continuation_exp = operation->eval(dummy_context);
    if (!continuation_exp)
        return Unexpected(PROPAGATE(continuation_exp));

    while (continuation_exp.value()) {
        continuation_exp = continuation_exp.value()->eval(dummy_context);
        if (!continuation_exp)
            return Unexpected(PROPAGATE(continuation_exp));
    }

    if (dummy_context.size() != 1)
        return Unexpected(ERROR("malformed constant expression"));

    return dummy_context.pop().i32;
}

} // namespace runtime
