#include "runtime/instance.hpp"
#include "runtime/handler.hpp"


namespace runtime {

/***********/
/* Handler */
/***********/

size_t Handler::allocateEpilogue(Instance& instance) {
    size_t idx;

    if (free_list_.empty()) {
        idx = epilogues_.size();
        epilogues_.push_back(nullptr);
    } else {
        idx = free_list_.top();
        free_list_.pop();
        epilogues_[idx] = nullptr;
    }

    return idx;
}

void Handler::freeEpilogue(size_t id) {
    epilogues_[id] = nullptr;
    free_list_.push(id);
}

/*******************/
/* SysCall Handler */
/*******************/

Continuation SysCall::action(Instance& instance) {
    Context& instance_ctx = instance.getActiveContext();
    int32_t n = instance_ctx.getLocal(0).i32;
    int32_t a1 = instance_ctx.getLocal(1).i32;
    int32_t a2 = instance_ctx.getLocal(2).i32;
    int32_t a3 = instance_ctx.getLocal(3).i32;
    int32_t a4 = instance_ctx.getLocal(4).i32;
    int32_t a5 = instance_ctx.getLocal(5).i32;
    int32_t a6 = instance_ctx.getLocal(6).i32;

    Instance& kernel = instance.getKernel();

    Context& kernel_ctx = kernel.getActiveContext();
    kernel_ctx.pushI32(instance_ctx.getPid());
    kernel_ctx.pushI32(n);
    kernel_ctx.pushI32(a1);
    kernel_ctx.pushI32(a2);
    kernel_ctx.pushI32(a3);
    kernel_ctx.pushI32(a4);
    kernel_ctx.pushI32(a5);
    kernel_ctx.pushI32(a6);

    instance.switchToKernel();

    size_t epilogue_idx = allocateEpilogue(instance);
    epilogues_[epilogue_idx] = std::make_shared<Epilogue>(epilogue_idx, instance, *this);

    kernel_ctx.getEpilogues().push(epilogues_[epilogue_idx].get());
    Function syscall_handler = kernel.getGlobalState().getFunction(handler_idx_);

    return syscall_handler.enterFrame(kernel_ctx);
}

Continuation SysCall::Epilogue::action(Instance& kernel) {
    Context& kernel_ctx = kernel.getActiveContext();

    Function syscall_handler =
        kernel.getGlobalState().getFunction(parent_.handler_idx_);
    syscall_handler.leaveFrame(kernel_ctx);

    Context& proc_ctx = suspended_instance_.getActiveContext();
    if (proc_ctx.getRunState() == Context::RunState::running) {
        int32_t result = kernel_ctx.pop().i32; // else this is the exit code
        proc_ctx.pushI32(result);
        kernel.switchToInstance(suspended_instance_);
    }

    parent_.freeEpilogue(id_);
    return nullptr;
}

/**********************/
/* PageFault Handler  */
/**********************/

Continuation PageFault::action(Instance& instance) {
    Context& instance_ctx = instance.getActiveContext();
    int32_t access_type = instance_ctx.pop().i32;
    int32_t faulting_address = instance_ctx.pop().i32;

    Instance& kernel = instance.getKernel();

    Context& kernel_ctx = kernel.getActiveContext();
    kernel_ctx.pushI32(instance_ctx.getPid());
    kernel_ctx.pushI32(faulting_address);
    kernel_ctx.pushI32(access_type);

    instance.switchToKernel();

    size_t epilogue_idx = allocateEpilogue(instance);
    epilogues_[epilogue_idx] = std::make_shared<Epilogue>(epilogue_idx, instance, *this);

    kernel_ctx.getEpilogues().push(epilogues_[epilogue_idx].get());
    Function page_fault_handler = kernel.getGlobalState().getFunction(handler_idx_);
    return page_fault_handler.enterFrame(kernel_ctx);
}

Continuation PageFault::Epilogue::action(Instance& kernel) {
    Context& kernel_ctx = kernel.getActiveContext();

    Function page_fault_handler =
        kernel.getGlobalState().getFunction(parent_.handler_idx_);
    page_fault_handler.leaveFrame(kernel_ctx);

    Context& proc_ctx = suspended_instance_.getActiveContext();
    if (proc_ctx.getRunState() == Context::RunState::running) {
        kernel.switchToInstance(suspended_instance_);
    }

    parent_.freeEpilogue(id_);
    return nullptr;
}

} // namespace runtime
