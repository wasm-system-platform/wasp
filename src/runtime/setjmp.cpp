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

        const Operation* epilogue_data = context.getEpilogues().data();
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

        const Operation* epilogue_data = context.getEpilogues().data();
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

    context.getEpilogues().push(epilogue_);

    uint32_t jbuffer_offset = context.getLocal(0).i32;
    int32_t savesigs = context.getLocal(1).i32;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.contains(jbuffer_offset, sizeof(JumpBuffer)))
        return trap(instance, "sigsetjmp out of bounds memory address", addr_);

    context.pushI32(jbuffer_offset);
    context.pushI32(jbuffer_offset);
    context.pushI32(savesigs);

    return prologue_call_.get();
}

Continuation Sigsetjmp::Epilogue::action(Instance& instance) {
    Context& context = instance.getActiveContext();
    
    uint32_t jbuffer_offset = context.pop().i32;

    JumpBuffer* jbuffer;

    Memory& memory = instance.getGlobalState().getMemory();
    if (!memory.ptr(jbuffer_offset, &jbuffer))
        return trap(instance, "sigsetjmp out of bounds memory address", addr_);

    jbuffer->stack_size = context.getStack().size();
    jbuffer->frames_size = context.getLocals().getFramePointersSize();
    jbuffer->epilogues_size = context.getEpilogues().size();
    jbuffer->setjmp_epilogue = epilogue_call_.get();
    jbuffer->return_epilogue = context.getEpilogues().peek().get();
    jbuffer->computeValidator(context);

    context.pushI32(0);
    context.pushI32(jbuffer_offset);
    context.pushI32(0);
    return epilogue_call_.get();
}

/***********/
/* Longjmp */
/***********/

Continuation Longjmp::action(Instance& instance) {
    Context& context = instance.getActiveContext();

    uint32_t jbuffer_offset = context.getLocal(0).i32;
    int32_t status = context.getLocal(1).i32;

    Memory& memory = instance.getGlobalState().getMemory();
    JumpBuffer* jbuffer;

    if (!memory.ptr(jbuffer_offset, &jbuffer))
        return trap(instance, "longjmp out of bounds memory address", addr_);

    if (jbuffer->stack_size != context.getStack().size())
        return trap(instance, "longjmp invalid program state: handle me", addr_);

    if (context.getLocals().getFramePointersSize() < jbuffer->frames_size)
        return trap(instance, "longjmp invalid program state: frame stack is too small", addr_);

    while (context.getLocals().getFramePointersSize() > jbuffer->frames_size) {
        context.getLocals().popLocals();
    }

    if (context.getEpilogues().size() < jbuffer->epilogues_size)
        return trap(instance, "longjmp invalid program state: epiloges stack is too small", addr_);

    context.getEpilogues().shrink(jbuffer->epilogues_size);
    context.getEpilogues().swap(jbuffer->return_epilogue->shared_from_this());

    if (!jbuffer->isValid(context))
        return trap(instance, "longjmp invalid program state: corrupt jump buffer", addr_);

    uint32_t ubuffer_offset = jbuffer_offset + offsetof(JumpBuffer, user_buffer);
    context.pushI32(status ? status : 1);
    context.pushI32(ubuffer_offset);
    context.pushI32(status ? status : 1);
    return jbuffer->setjmp_epilogue;
}

}