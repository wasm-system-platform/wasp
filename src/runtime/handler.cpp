#include "runtime/handler.hpp"
#include "runtime/instance.hpp"
#include "runtime/interrupts.hpp"
#include "runtime/kernel.hpp"

namespace runtime {

/*********************/
/* Interrupt Handler */
/*********************/

Continuation Interrupt::action(Instance& instance) {
    Context& instance_ctx = instance.getActiveContext();

    controller_.disableInterrupts();

    auto epilogue = std::make_shared<Epilogue>(instance, *this);

    if (instance.is<Kernel>()) {
        // if we got interrupted in the kernel we can just call the handler
        // directly
        Kernel& kernel = instance.as<Kernel>();
        Function interrupt_handler =
            kernel.getGlobalState().getFunction(handler_idx_);

        instance_ctx.getEpilogues().push(epilogue);

        return interrupt_handler.enterFrame(instance_ctx);
    } else {
        Kernel& kernel = instance.as<Process>().getKernel();
        Function interrupt_handler =
            kernel.getGlobalState().getFunction(handler_idx_);

        uint32_t port = instance_ctx.getStack().pop().i32;

        Context& kernel_ctx = instance.getActiveContext();
        kernel_ctx.pushI32(port);
        instance_ctx.getEpilogues().push(epilogue);

        kernel.switchBack();

        return interrupt_handler.enterFrame(kernel_ctx);
    }
}

Continuation Interrupt::Epilogue::action(Instance& instance) {
    parent_.controller_.enableInterrupts();

    Context& kernel_ctx = instance.getActiveContext();
    Function syscall_handler =
        instance.getGlobalState().getFunction(parent_.handler_idx_);
    syscall_handler.leaveFrame(kernel_ctx);

    if (suspended_instance_.is<Process>()) {
        instance.as<Kernel>().switchToInstance(suspended_instance_);
    }

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

    Context& kernel_ctxt = kernel.getActiveContext();
    kernel_ctxt.pushI32(instance.as<Process>().getId());
    kernel_ctxt.pushI32(n);
    kernel_ctxt.pushI32(a1);
    kernel_ctxt.pushI32(a2);
    kernel_ctxt.pushI32(a3);
    kernel_ctxt.pushI32(a4);
    kernel_ctxt.pushI32(a5);
    kernel_ctxt.pushI32(a6);

    kernel.switchBack();

    auto epilogue = std::make_shared<Epilogue>(instance, *this);
    kernel_ctxt.getEpilogues().push(epilogue);
    Function syscall_handler =
        kernel.getGlobalState().getFunction(handler_idx_);

    return syscall_handler.enterFrame(kernel_ctxt);
}

Continuation SysCall::Epilogue::action(Instance& instance) {
    Context& kernel_ctxt = instance.getActiveContext();
    int32_t result = kernel_ctxt.pop().i32;

    Function syscall_handler =
        instance.getGlobalState().getFunction(parent_.handler_idx_);
    syscall_handler.leaveFrame(kernel_ctxt);

    Context& proc_ctxt = suspended_instance_.getActiveContext();
    proc_ctxt.setRunState(Context::RunState::running);
    proc_ctxt.pushI32(result);

    /*
    fmt::println("syscall returned: {}", result);
    const Operation* epilogues = proc_ctxt.getEpilogues().data();
    for (size_t i = proc_ctxt.getEpilogues().size() - 1; i > 0; i--) {
        const Operation& epilogue = epilogues[i];
        if (epilogue == nullptr) break;
        fmt::println("  {}: at {}", i,
    epilogue->getFormattedAddress(suspended_instance_));
    }
    */

    instance.as<Kernel>().switchToInstance(suspended_instance_);

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

    // allocate epilogue
    auto epilogue = std::make_shared<Epilogue>(instance, *this);

    // pagefault is triggered by a process
    if (instance.is<Process>()) {
        // push arguments on the kernel stack
        Kernel& kernel = instance.as<Process>().getKernel();
        Context& kernel_ctx = kernel.getActiveContext();
        kernel_ctx.pushI32(instance.as<Process>().getId());
        kernel_ctx.pushI32(faulting_address);
        kernel_ctx.pushI32(access_type);

        // push epilogue to kernel epilogues
        kernel_ctx.getEpilogues().push(epilogue);

        // switch to kernel instance
        kernel.switchBack();

        // call page fault handler
        Function page_fault_handler =
            kernel.getGlobalState().getFunction(handler_idx_);
        return page_fault_handler.enterFrame(kernel_ctx);
    }
    // pagefault is triggered by the kernel
    else {
        // push arguments back to the instance stack
        instance_ctx.pushI32(-1);
        instance_ctx.pushI32(faulting_address);
        instance_ctx.pushI32(access_type);

        // push epilogue to instance epilogues
        instance_ctx.getEpilogues().push(epilogue);

        // call page fault handler
        Function page_fault_handler =
            instance.getGlobalState().getFunction(handler_idx_);
        return page_fault_handler.enterFrame(instance_ctx);
    }
}

Continuation PageFault::Epilogue::action(Instance& instance) {
    Kernel& kernel = instance.as<Kernel>();
    Context& kernel_ctxt = instance.getActiveContext();

    Function page_fault_handler =
        instance.getGlobalState().getFunction(parent_.handler_idx_);
    page_fault_handler.leaveFrame(kernel_ctxt);

    if (suspended_instance_.is<Process>()) {
        Context& proc_ctx = suspended_instance_.getActiveContext();
        assert(proc_ctx.getRunState() == Context::RunState::running);
        kernel.switchToInstance(suspended_instance_);
    }

    return nullptr;
}

} // namespace runtime
