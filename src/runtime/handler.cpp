#include <utility>

#include "runtime/handler.hpp"
#include "runtime/instance.hpp"
#include "runtime/interrupts.hpp"
#include "runtime/kernel.hpp"

namespace runtime {

/*********************/
/* Interrupt Handler */
/*********************/

Continuation Interrupt::action(Instance& instance) {
    Context& interrupt_ctx = instance.getActiveContext();

    Kernel& kernel = instance.is<Kernel>() ? instance.as<Kernel>()
                                           : instance.as<Process>().getKernel();
    kernel.suspend(instance);

    Function& interrupt_handler =
        kernel.getGlobalState().getFunction(handler_idx_);

    uint32_t pid = instance.is<Kernel>() ? -1 : instance.as<Process>().getId();
    uint32_t port = static_cast<uint32_t>(interrupt_ctx.getStack().pop().i32);

    Context& kernel_ctx = kernel.getActiveContext();

    kernel_ctx.pushI32(pid);
    kernel_ctx.pushI32(static_cast<int32_t>(port));
    kernel_ctx.getEpilogues().push(&epilogue_);

    controller_.disableInterrupts();
    return interrupt_handler.enterFrame(kernel_ctx);
}

Continuation Interrupt::Epilogue::action(Instance& instance) {
    Kernel& kernel = instance.as<Kernel>();
    kernel.resume();

    Context& kernel_ctx = kernel.getActiveContext();
    Function& syscall_handler =
        kernel.getGlobalState().getFunction(parent_.handler_idx_);
    syscall_handler.leaveFrame(kernel_ctx);

    parent_.controller_.enableInterrupts();
    return nullptr;
}

/*******************/
/* SysCall Handler */
/*******************/

Continuation SysCall::action(Instance& instance) {
    Context& instance_ctxt = instance.getActiveContext();
    instance_ctxt.setRunState(Context::RunState::in_syscall);

    int32_t n = instance_ctxt.getLocal(0).i32;
    int32_t a1 = instance_ctxt.getLocal(1).i32;
    int32_t a2 = instance_ctxt.getLocal(2).i32;
    int32_t a3 = instance_ctxt.getLocal(3).i32;
    int32_t a4 = instance_ctxt.getLocal(4).i32;
    int32_t a5 = instance_ctxt.getLocal(5).i32;
    int32_t a6 = instance_ctxt.getLocal(6).i32;

    Kernel& kernel = instance.as<Process>().getKernel();
    kernel.suspend(instance);

    Context& kernel_ctxt = kernel.getActiveContext();
    kernel_ctxt.pushI32(static_cast<int32_t>(instance.as<Process>().getId()));
    kernel_ctxt.pushI32(n);
    kernel_ctxt.pushI32(a1);
    kernel_ctxt.pushI32(a2);
    kernel_ctxt.pushI32(a3);
    kernel_ctxt.pushI32(a4);
    kernel_ctxt.pushI32(a5);
    kernel_ctxt.pushI32(a6);

    kernel_ctxt.getEpilogues().push(&epilogue_);
    Function& syscall_handler =
        kernel.getGlobalState().getFunction(handler_idx_);

    return syscall_handler.enterFrame(kernel_ctxt);
}

Continuation SysCall::Epilogue::action(Instance& instance) {
    Kernel& kernel = instance.as<Kernel>();
    Context& kernel_ctxt = kernel.getActiveContext();
    int32_t result = kernel_ctxt.pop().i32;

    Function& syscall_handler =
        instance.getGlobalState().getFunction(parent_.handler_idx_);
    syscall_handler.leaveFrame(kernel_ctxt);

    Instance& proc = kernel.resume();
    Context& proc_ctxt = proc.getActiveContext();
    proc_ctxt.setRunState(Context::RunState::running);
    proc_ctxt.pushI32(result);

    return nullptr;
}

/**********************/
/* PageFault Handler  */
/**********************/

Continuation PageFault::action(Instance& instance) {
    // pop all arguments from the faulting instance
    Context& instance_ctx = instance.getActiveContext();
    int32_t access_type = instance_ctx.pop().i32;
    int32_t faulting_address = instance_ctx.pop().i32;

    uint32_t pid =
        instance.is<Kernel>() ? UINT32_MAX : instance.as<Process>().getId();

    Kernel& kernel = instance.is<Kernel>() ? instance.as<Kernel>()
                                           : instance.as<Process>().getKernel();
    kernel.suspend(instance);

    Context& kernel_ctx = kernel.getActiveContext();
    kernel_ctx.pushI32(pid);
    kernel_ctx.pushI32(faulting_address);
    kernel_ctx.pushI32(access_type);
    kernel_ctx.getEpilogues().push(&epilogue_);

    Function& page_fault_handler =
        kernel.getGlobalState().getFunction(handler_idx_);
    return page_fault_handler.enterFrame(kernel_ctx);
}

Continuation PageFault::Epilogue::action(Instance& instance) {
    Kernel& kernel = instance.as<Kernel>();
    kernel.resume();

    Context& kernel_ctxt = instance.getActiveContext();
    Function& page_fault_handler =
        instance.getGlobalState().getFunction(parent_.handler_idx_);
    page_fault_handler.leaveFrame(kernel_ctxt);

    return nullptr;
}

/******************/
/* Signal Handler */
/******************/

Continuation Signal::action(Instance& instance) {
    Kernel& kernel = instance.as<Kernel>();
    Context& kernel_ctxt = kernel.getActiveContext();

    uint32_t pid = static_cast<uint32_t>(kernel_ctxt.getLocal(0).i32);
    uint32_t handler_idx = static_cast<uint32_t>(kernel_ctxt.getLocal(1).i32);
    int32_t signal = kernel_ctxt.getLocal(2).i32;

    ProcessManager& proc_mgr = kernel.getProcessManager();
    std::shared_ptr<Process> proc;

    Errno result = proc_mgr.getProcess(pid, proc);
    if (result != Errno::success) {
        kernel_ctxt.pushI32(std::to_underlying(result));
        fmt::println("wrong pid");
        return nullptr;
    }

    /* Find process handler function. */
    std::vector<Function>& functions = proc->getGlobalState().getFunctions();
    std::vector<uint32_t>& indirections =
        proc->getGlobalState().getIndirections();

    if (handler_idx >= indirections.size() ||
        indirections[handler_idx] >= functions.size()) {
        kernel_ctxt.pushI32(std::to_underlying(Errno::invalid));
        fmt::println("wrong handler addr");
        return nullptr;
    }

    /* Handler must be of type (i32) -> (). */
    Function& func = functions[indirections[handler_idx]];
    if (func.getSignature() != FunctionType::ConsumerI32().getSignature()) {
        kernel_ctxt.pushI32(std::to_underlying(Errno::invalid));
        fmt::println("wrong signature");
        return nullptr;
    }

    /* Prepare process state. */
    Context& proc_ctxt = proc->getActiveContext();
    proc_ctxt.pushI32(handler_idx);
    proc_ctxt.getStack().push(signal);
    proc_ctxt.getEpilogues().push(&epilogue_);

    /* Switch to process and call the handler function. */
    kernel.signal(*proc);
    return func.enterFrame(proc_ctxt);
}

Continuation Signal::Epilogue::action(Instance& instance) {
    Context& ctxt = instance.getActiveContext();
    uint32_t handler_idx = ctxt.pop().i32;

    std::vector<Function>& functions = instance.getGlobalState().getFunctions();
    std::vector<uint32_t>& indirections =
        instance.getGlobalState().getIndirections();

    Function& func = functions[indirections[handler_idx]];
    func.leaveFrame(instance.getActiveContext());

    Kernel& kernel = instance.as<Process>().getKernel();
    kernel.getActiveContext().getStack().push(
        std::to_underlying(Errno::success));
    kernel.resume();
    return nullptr;
}

} // namespace runtime
