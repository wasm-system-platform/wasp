#include <sstream>

#include "runtime/process.hpp"
#include "runtime/kernel.hpp"
#include "runtime/setjmp.hpp"
#include "runtime/context_manager.hpp"

namespace runtime {

Expected<std::shared_ptr<Process>>
Process::create(const std::string &program, Instance &instance, uint32_t id) {
    std::stringstream program_stream(program);

    Expected<grammar::Module> module_exp =
        grammar::Module::parse(program_stream);
    if (!module_exp)
        return Unexpected(PROPAGATE(module_exp));

    // create imports to be supplied
    std::shared_ptr<Sigsetjmp> sigsetjmp;
    Expected<Imports> imports_exp = createImports(instance, sigsetjmp);
    if (!imports_exp)
        return Unexpected(PROPAGATE(imports_exp));

    // create global state (functions, memories, debug info, ...)
    Expected<GlobalState> global_state_exp = GlobalState::create(*module_exp, *imports_exp);
    if (!global_state_exp)
        return Unexpected(PROPAGATE(global_state_exp));

    // create exports (entry points)
    Expected<std::shared_ptr<Exports>> exports_exp = Instance::createExports(*module_exp);
    if (!exports_exp)
        return Unexpected(PROPAGATE(exports_exp));
    
    class Builder : public Process {
    public:
        Builder(GlobalState&& global_state, std::shared_ptr<Exports>& exports, const grammar::Module& module, Kernel& kernel, uint32_t id)
            : Process(std::move(global_state), exports, module, kernel, id) {}
    };
    std::shared_ptr<Process> proc = std::make_shared<Builder>(std::move(*global_state_exp), *exports_exp, *module_exp, 
        instance.as<Kernel>(), id);

    Expected<void> result = proc->validateExports(sigsetjmp);
    if (!result)
        return Unexpected(PROPAGATE(result));

    return proc;
}

Errno Process::clone(uint32_t new_id, std::shared_ptr<Process>& proc_out) {
    ContextManager& ctx_mgr = ContextManager::instance();

    uint32_t clone_cid;
    Errno result = ctx_mgr.cloneContext(getActiveContext().getId(), clone_cid);
    if (result != Errno::SUCCESS)
        return result;
    
    class Builder : public Process {
    public:
        Builder(Process& process, uint32_t new_id) : Process(process, new_id) {}
    };

    proc_out = std::make_shared<Builder>(*this, new_id);
    proc_out->setActiveContext(ctx_mgr.getContext(clone_cid));
    return Errno::SUCCESS;
}

Expected<Imports> Process::createImports(Instance& instance, std::shared_ptr<Sigsetjmp>& sigsetjmp_out) {
    auto it = instance.exports_->find("sys_call_handler");
    if (it == instance.exports_->end()) {
        return Unexpected(ERROR("kernel export 'sys_call_handler' not found"));
    }

    Export& sys_call_handler = it->second;

    std::shared_ptr<SysCall> sys_call_op =
        std::make_shared<SysCall>(sys_call_handler.func_idx);

    Function sys_call(std::move(sys_call_op), 7,
                      {int32_t(0), int32_t(0), int32_t(0), int32_t(0),
                       int32_t(0), int32_t(0), int32_t(0)},
                      FunctionType::ProducerI32x7().getSignature());

    std::function<int32_t(Instance&)> sys_execve_stack_func =
        [](Instance& instance) -> int32_t {
        return instance.as<Process>().getExecveStack();
    };

    Function sys_execve_stack = Function::createExternal(sys_execve_stack_func);

    // setjmp/longjmp
    sigsetjmp_out = std::make_shared<Sigsetjmp>();
    Function env_sigsetjmp(sigsetjmp_out, 2, {int32_t(0), int32_t(0)}, FunctionType::ProducerI32x2().getSignature());
    
    std::shared_ptr<Longjmp> env_longjmp_op = std::make_shared<Longjmp>();
    Function env_longjmp(env_longjmp_op, 2, {int32_t(0), int32_t(0)}, FunctionType::ConsumerI32x2().getSignature());
 
    return Imports{
        {"sys.call", sys_call},
        {"sys.execve_stack", sys_execve_stack},
        {"env.sigsetjmp", env_sigsetjmp},
        {"env.longjmp", env_longjmp},
    };
}

Expected<void> Process::validateExports(std::shared_ptr<Sigsetjmp>& sigsetjmp) {
    auto it = exports_->find("_start");
    if (it != exports_->end()) {
        Export& start = it->second;
        size_t start_sig = FunctionType::Empty().getSignature();

        if (start_sig == start.signature) {
            entry_ = std::make_shared<Call>(start.func_idx, UINT32_MAX);
        } else {
            return Unexpected(ERROR(fmt::format("warning: _start has a wrong signature; "
                              "expected signature '{}' but found '{}'",
                              FunctionType::ConsumerI32x2().toString(),
                              start.func_type.toString()).c_str()));
        }
    }

    it = exports_->find("sigsetjmp_prologue");
    if (it != exports_->end()) {
        Export& sigsetjmp_prologue = it->second;
        size_t sigsetjmp_prologue_sig = FunctionType::ConsumerI32x2().getSignature();

        if (sigsetjmp_prologue_sig == sigsetjmp_prologue.signature) {
            sigsetjmp->setPrologueFuncIdx(sigsetjmp_prologue.func_idx);
        } else {
            fmt::println("warning: sigsetjmp prologue has a wrong signature; "
                              "expected signature '{}' but found '{}'",
                              FunctionType::ConsumerI32x2().toString(),
                              sigsetjmp_prologue.func_type.toString());
        }
    }

    it = exports_->find("sigsetjmp_epilogue");
    if (it != exports_->end()) {
        Export& sigsetjmp_epilogue = it->second;
        size_t sigsetjmp_epilogue_sig = FunctionType::ConsumerI32x2().getSignature();

        if (sigsetjmp_epilogue_sig == sigsetjmp_epilogue.signature) {
            sigsetjmp->setEpilogueFuncIdx(sigsetjmp_epilogue.func_idx);
        } else {
            fmt::println("warning: sigsetjmp epilogue has a wrong signature; "
                              "expected signature '{}' but found '{}'",
                              FunctionType::ConsumerI32x2().toString(),
                              sigsetjmp_epilogue.func_type.toString());
        }
    }

    return {};
}

}
