#include <fmt/format.h>

#include "runtime/vm.hpp"

namespace runtime {

/*******************/
/* Memory Instance */
/*******************/

Expected<MemoryInstances>
MemoryInstances::create(const grammar::Module& module) {
    const grammar::ImportSection& import_section = module.getImportSection();
    const std::vector<grammar::MemoryImport> mem_imports =
        import_section.getMemoryImports();

    if (mem_imports.empty())
        return MemoryInstances({}, 0, false);

    if (mem_imports.size() > 1)
        return Unexpected(ERROR("multi memory is currently not supported"));

    constexpr size_t PAGE_SIZE = 1024 * 64;

    const MemoryType& type = mem_imports[0].getMemoryType();
    std::vector<uint8_t> heap(type.getInitial() * PAGE_SIZE);

    return MemoryInstances(std::move(heap), type.getMax(), type.isShared());
}

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
    const std::vector<grammar::Data>& data_segments =
        data_section.getSegments();

    std::vector<std::vector<uint8_t>> segments;
    for (const grammar::Data& segment : data_segments) {
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

    Expected<MemoryInstances> mems_exp = MemoryInstances::create(module);
    if (!mems_exp)
        return Unexpected(PROPAGATE(mems_exp));

    Expected<GlobalInstances> global_exp = GlobalInstances::create(module);
    if (!global_exp)
        return Unexpected(PROPAGATE(global_exp));

    Expected<DataInstances> datas_exp = DataInstances::create(module);
    if (!datas_exp)
        return Unexpected(PROPAGATE(datas_exp));

    return GlobalState(std::move(*funcs_exp), std::move(*indirect_funcs_exp),
                       *mems_exp, *global_exp, *datas_exp);
}

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

    return dummy_context.popI32();
}

} // namespace runtime
