#include <fmt/format.h>

#include "runtime/context_manager.hpp"
#include "runtime/instance.hpp"

namespace runtime {

Expected<std::shared_ptr<Instance>>
Instance::create(const grammar::Module& module,
                 const std::unordered_map<std::string, Function>& imports) {
    Expected<GlobalState> store_exp = GlobalState::create(module, imports);
    if (!store_exp)
        return Unexpected(PROPAGATE(store_exp));

    Expected<std::shared_ptr<Exports>> exports_exp = createExports(module);
    if (!exports_exp)
        return Unexpected(PROPAGATE(exports_exp));

    return std::make_shared<Instance>(
        Instance(std::move(*store_exp), *exports_exp, module));
}

Expected<void> Instance::call(const std::string& name) {
    Expected<void> result = call(name, FunctionType::Empty());
    if (!result)
        return Unexpected(PROPAGATE(result));

    return {};
};

Expected<void> Instance::call(const std::string& name, int32_t a) {
    ContextManager& ctxt_manager = ContextManager::instance();
    Context& ctxt = ctxt_manager.createEmpty();
    active_context_ = &ctxt;
    active_context_->pushI32(a);

    Expected<void> result = call(name, FunctionType::ConsumerI32());
    if (!result)
        return Unexpected(PROPAGATE(result));

    active_context_ = nullptr;
    ctxt_manager.destroyContext(ctxt.getId());
    return {};
}

Expected<void> Instance::call(const std::string& name, int32_t a, int32_t b) {
    active_context_->pushI32(a);
    active_context_->pushI32(b);

    Expected<void> result = call(name, FunctionType::ConsumerI32x2());
    if (!result)
        return Unexpected(PROPAGATE(result));

    return {};
}

Expected<int32_t> Instance::callRet(const std::string& name, int32_t a,
                                    int32_t b) {
    ContextManager& ctxt_manager = ContextManager::instance();
    Context& ctxt = ctxt_manager.createEmpty();
    active_context_ = &ctxt;
    active_context_->pushI32(a);
    active_context_->pushI32(b);

    Expected<void> result = call(name, FunctionType::ProducerI32x2());
    if (!result)
        return Unexpected(PROPAGATE(result));

    return active_context_->pop().i32;
}

Expected<std::shared_ptr<Instance::Exports>>
Instance::createExports(const grammar::Module& module) {
    grammar::TypeSection type_section = module.getTypeSection();
    grammar::ImportSection import_section = module.getImportSection();
    grammar::FunctionSection function_section = module.getFunctionSection();
    grammar::ExportSection export_section = module.getExportSection();

    std::vector<uint32_t> type_indices = function_section.getTypes();
    std::hash<FunctionType> signature_hasher;

    std::shared_ptr<Exports> exports = std::make_shared<Exports>();
    for (const auto& func : export_section.getFunctionExports()) {
        uint32_t type_index =
            type_indices[func.getIdx() -
                         import_section.getFunctionImports().size()];

        Expected<FunctionType> func_type_exp =
            type_section.getFunctionType(type_index);
        if (!func_type_exp)
            return Unexpected(PROPAGATE(func_type_exp));
        const FunctionType& func_type = *func_type_exp;

        size_t signature = signature_hasher(func_type);
        exports->emplace(func.getName(),
                         Export{func.getIdx(), func_type, signature});
    }

    return exports;
}

Instance::Instance(GlobalState&& global_state,
                   std::shared_ptr<Exports>& exports,
                   const grammar::Module& module)
    : exports_(exports), global_state_(std::move(global_state)) {

    if (module.hasStartSection()) {
        ContextManager& ctxt_manager = ContextManager::instance();
        Context& ctxt = ctxt_manager.createEmpty();
        active_context_ = &ctxt;

        uint32_t start_function = module.getStartSection().getFunctionIdx();
        invoke(start_function);

        active_context_ = nullptr;
        ctxt_manager.destroyContext(ctxt.getId());
    }
}

Expected<void> Instance::call(const std::string& name,
                              const FunctionType& func_type) {
    auto it = exports_->find(name);
    if (it == exports_->end())
        return Unexpected(ERROR(fmt::format(
            "call failed: function '{}' is not exported by the module", name)));
    const Export& exp = it->second;

    size_t signature = std::hash<FunctionType>()(func_type);
    if (exp.signature != signature) {
        return Unexpected(ERROR(
            fmt::format("call failed: signature mismatch when invoking '{}';  "
                        "expected signature '{}' but found '{}'",
                        name, exp.func_type.toString(), func_type.toString())));
    }

    if (active_context_->getRunState() != Context::RunState::rdy)
        return Unexpected(
            ERROR("call failed: stack is not in a runnable state"));

    invoke(exp.func_idx);

    return {};
}

void Instance::invoke(uint32_t func_idx) {
    Call call(func_idx, UINT32_MAX);
    run(call);
}

void Instance::invokeIndirect(uint32_t element_idx, size_t signature) {
    active_context_->pushI32(static_cast<int32_t>(element_idx));
    CallIndirect call_indirect(signature);
    run(call_indirect);
}

void Instance::run(OperationBase& entry) {
    assert(active_context_);
    Context::RunState state = active_context_->getRunState();

    assert(state == Context::RunState::rdy);
    active_context_->setRunState(Context::RunState::running);

    active_context_->getEpilogues().push(nullptr);

    Continuation continuation = entry.action(*this);
    while (continuation) {
        continuation = continuation->action(*this);

        // check for epilogues
        if (!continuation)
            continuation = active_context_->getEpilogues().pop().get();
    }

    assert(active_context_->getEpilogues().size() == 0);

    // reset program
    active_context_->getEpilogues().push(nullptr);
    active_context_->setRunState(Context::RunState::rdy);
}

} // namespace runtime
