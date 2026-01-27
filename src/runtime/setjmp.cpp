#include "runtime/setjmp.hpp"
#include "runtime/instance.hpp"

namespace runtime {

struct JumpBuffer {
    int32_t user_buffer[32];
    size_t stack_size;
    size_t frames_size;
    size_t epilogues_size;
    Continuation setjmp_epilogue;
    Continuation return_epilogue;
    size_t validator;
    
    void computeValidator(Context& context) {
        size_t hash = 0;
        hash_combine(hash, stack_size);
        hash_combine(hash, frames_size);
        hash_combine(hash, epilogues_size);
        hash_combine(hash, setjmp_epilogue);
        hash_combine(hash, return_epilogue);

        const Value* stack_data = context.getStack().data();
        for (size_t i = 0; i < context.getStack().size(); i++) {
            hashCombine(hash, stack_data[i].i64);
        }

        const Continuation* epilogue_data = context.getEpilogues().data();
        for (size_t i = 0; i < context.getEpilogues().size(); i++) {
            hashCombine(hash, epilogue_data[i]);
        }

        validator = hash;
    }

    bool isValid(Context& context) const {
        size_t hash = 0;
        hash_combine(hash, stack_size);
        hash_combine(hash, frames_size);
        hash_combine(hash, epilogues_size);
        hash_combine(hash, setjmp_epilogue);
        hash_combine(hash, return_epilogue);

        const Value* stack_data = context.getStack().data();
        for (size_t i = 0; i < context.getStack().size(); i++) {
            hashCombine(hash, stack_data[i].i64);
        }

        const Continuation* epilogue_data = context.getEpilogues().data();
        for (size_t i = 0; i < context.getEpilogues().size(); i++) {
            hashCombine(hash, epilogue_data[i]);
        }

        return validator == hash;
    }

private:
    template <class T>
    void hashCombine(size_t& hash, const T& v) const {
        std::hash<T> hasher;
        hash ^= hasher(v) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
    }
};

/*************/
/* Sigsetjmp */
/*************/

Continuation Sigsetjmp::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    context.getEpilogues().push(epilogue_.get());

    uint32_t jb_addr = context.getLocal(0).i32;
    int32_t savesigs = context.getLocal(1).i32;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (jb_addr + sizeof(JumpBuffer) > memory.size())
        return trap(instance, "sigsetjmp out of bounds memory address", addr_);

    context.pushI32(jb_addr);
    context.pushI32(jb_addr);
    context.pushI32(savesigs);

    return prologue_call_.get();
}

Continuation Sigsetjmp::Epilogue::action(Instance& instance) {
    Context& context = instance.getActiveContext();
    
    uint32_t jb_addr = context.pop().i32;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (jb_addr + sizeof(JumpBuffer) > memory.size())
        return trap(instance, "sigsetjmp out of bounds memory address", addr_);

    JumpBuffer* jb = reinterpret_cast<JumpBuffer*>(&memory[jb_addr]);
    jb->stack_size = context.getStack().size();
    jb->frames_size = context.getLocals().getFramePointersSize();
    jb->epilogues_size = context.getEpilogues().size();
    jb->setjmp_epilogue = epilogue_call_.get();
    jb->computeValidator(context);

    fmt::print("sigsetjmp action: stack_size={}, frames_size={}, epilogues_size={}", jb->stack_size, jb->frames_size, jb->epilogues_size);

    context.pushI32(0);
    context.pushI32(jb_addr);
    context.pushI32(0);
    return epilogue_call_.get();
}

/***********/
/* Longjmp */
/***********/

Continuation Longjmp::action(Instance& instance) {
    fmt::println("longjmp action");
    Context& context = instance.getActiveContext();

    uint32_t jb_addr = context.getLocal(0).i32;
    int32_t status = context.getLocal(1).i32;

    std::vector<uint8_t>& memory = instance.getGlobalState().getMemory();
    if (jb_addr + sizeof(JumpBuffer) > memory.size())
        return trap(instance, "longjmp out of bounds memory address", addr_);

    JumpBuffer* jb = reinterpret_cast<JumpBuffer*>(&memory[jb_addr]);
    fmt::println("longjmp action: stack_size={}, frames_size={}, epilogues_size={}", jb->stack_size, jb->frames_size, jb->epilogues_size);


    if (jb->stack_size != context.getStack().size())
        return trap(instance, "longjmp invalid program state: handle me", addr_);

    fmt::println("longjmp action: frames_size={}", context.getLocals().getFramePointersSize());

    if (context.getLocals().getFramePointersSize() < jb->frames_size)
        return trap(instance, "longjmp invalid program state: frame stack is too small", addr_);

    while (context.getLocals().getFramePointersSize() > jb->frames_size) {
        context.getLocals().popLocals();
    }

    if (context.getEpilogues().size() < jb->epilogues_size)
        return trap(instance, "longjmp invalid program state: epiloges stack is too small", addr_);

    context.getEpilogues().shrink(jb->epilogues_size);
    context.getEpilogues().swap(jb->return_epilogue);

    if (!jb->isValid(context))
        return trap(instance, "longjmp invalid program state: corrupt jump buffer", addr_);

    uint32_t ub_addr = jb_addr + offsetof(JumpBuffer, user_buffer);
    context.pushI32(ub_addr);
    context.pushI32(status ? status : 1);
    return jb->setjmp_epilogue;
}

}