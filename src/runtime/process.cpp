#include "runtime/checkpoint.hpp"
#include "runtime/context_manager.hpp"
#include "runtime/kernel.hpp"
#include "util/memory_stream.hpp"

namespace runtime {

Expected<std::shared_ptr<Process>>
Process::create(std::span<const char>& program_bytes, Instance& instance,
                uint32_t id) {
    MemoryIStream program_stream(program_bytes.data(), program_bytes.size());

    Expected<grammar::Module> module_exp =
        grammar::Module::parse(program_stream);
    if (!module_exp)
        return Unexpected(PROPAGATE(module_exp));

    // create imports to be supplied
    Expected<Imports> imports_exp = createImports(instance);
    if (!imports_exp)
        return Unexpected(PROPAGATE(imports_exp));

    // create global state (functions, memories, debug info, ...)
    Expected<GlobalState> global_state_exp =
        GlobalState::create(*module_exp, *imports_exp);
    if (!global_state_exp)
        return Unexpected(PROPAGATE(global_state_exp));

    // create exports (entry points)
    Expected<std::shared_ptr<Exports>> exports_exp =
        Instance::createExports(*module_exp);
    if (!exports_exp)
        return Unexpected(PROPAGATE(exports_exp));

    class Builder : public Process {
    public:
        Builder(GlobalState&& global_state, std::shared_ptr<Exports>& exports,
                const grammar::Module& module, Kernel& kernel, uint32_t id)
            : Process(std::move(global_state), exports, module, kernel, id) {}
    };
    std::shared_ptr<Process> proc =
        std::make_shared<Builder>(std::move(*global_state_exp), *exports_exp,
                                  *module_exp, instance.as<Kernel>(), id);

    Expected<void> result = proc->validateExports();
    if (!result)
        return Unexpected(PROPAGATE(result));

    return proc;
}

Errno Process::clone(uint32_t new_id, std::shared_ptr<Process>& proc_out) {
    ContextManager& ctx_mgr = ContextManager::instance();

    uint32_t clone_cid;
    Errno result = ctx_mgr.cloneContext(getActiveContext().getId(), clone_cid);
    if (result != Errno::success)
        return result;

    class Builder : public Process {
    public:
        Builder(Process& process, uint32_t new_id) : Process(process, new_id) {}
    };

    proc_out = std::make_shared<Builder>(*this, new_id);
    proc_out->setActiveContext(ctx_mgr.getContext(clone_cid));
    return Errno::success;
}

Expected<Imports>
Process::createImports(Instance& instance) {
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
        return static_cast<int32_t>(instance.as<Process>().getExecveStack());
    };

    Function sys_execve_stack = Function::createExternal(sys_execve_stack_func);

    // save/restore
    std::function<int32_t(Instance & instance, int32_t, int32_t)>
        env_save_func = [](Instance& instance, uint32_t checkpoint_offset,
                           uint32_t checkpoint_len) -> int32_t {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.contains(checkpoint_offset, checkpoint_len))
            return static_cast<int32_t>(Errno::invalid);

        uint8_t* checkpoint_ptr;
        memory.ptr(checkpoint_offset, &checkpoint_ptr);
        std::span<uint8_t> checkpoint(checkpoint_ptr, checkpoint_len);

        return static_cast<int32_t>(checkpoint::create(instance, checkpoint));
    };
    std::function<int32_t(Instance & instance, int32_t, int32_t)>
        env_restore_func = [](Instance& instance, uint32_t checkpoint_offset,
                              uint32_t checkpoint_len) -> int32_t {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.contains(checkpoint_offset, checkpoint_len))
            return static_cast<int32_t>(Errno::invalid);

        const uint8_t* checkpoint_ptr;
        memory.ptr(checkpoint_offset, &checkpoint_ptr);
        std::span<const uint8_t> checkpoint(checkpoint_ptr, checkpoint_len);

        return static_cast<int32_t>(checkpoint::restore(instance, checkpoint));
    };

    Function env_save = Function::createExternal(env_save_func);
    Function env_restore = Function::createExternal(env_restore_func);

    return Imports{
        {"sys.call", sys_call},
        {"sys.execve_stack", sys_execve_stack},
        {"env.save", env_save},
        {"env.restore", env_restore},
    };
}

Expected<void> Process::validateExports() {
    auto it = exports_->find("_start");
    if (it != exports_->end()) {
        Export& start = it->second;
        size_t start_sig = FunctionType::Empty().getSignature();

        if (start_sig == start.signature) {
            entry_ = std::make_shared<Call>(start.func_idx, UINT32_MAX);
        } else {
            return Unexpected(
                ERROR(fmt::format("warning: _start has a wrong signature; "
                                  "expected signature '{}' but found '{}'",
                                  FunctionType::ConsumerI32x2().toString(),
                                  start.func_type.toString())
                          .c_str()));
        }
    }

    return {};
}

} // namespace runtime
