#pragma once

#include <cassert>
#include <cxxabi.h>
#include <stack>

#include "grammar/sections.hpp"

namespace runtime {

struct Value {
    union {
        int32_t i32;
        int64_t i64;
        float f32;
        double f64;
    };

    Value() = default;
    Value(int32_t i32) noexcept : i32(i32) {}
    Value(int64_t i64) noexcept : i64(i64) {}
    Value(float f32) noexcept : f32(f32) {}
    Value(double f64) noexcept : f64(f64) {}
};
static_assert(std::is_trivially_copyable_v<Value>);

template <std::size_t num> struct ValueTuple {
    template <std::size_t> using AlwaysValue = Value;

    template <std::size_t... I>
    static auto make(std::index_sequence<I...>)
        -> std::tuple<AlwaysValue<I>...>;

    using type = decltype(make(std::make_index_sequence<num>{}));
};

template <> struct ValueTuple<1> {
    using type = Value;
};

template <> struct ValueTuple<0> {
    using type = void;
};

template <std::size_t num> using Values = typename ValueTuple<num>::type;

class Instance;
class Context;
class GlobalState;
class Function;

class OperationBase;
using Operation = OperationBase*;
using Operations = std::vector<Operation>;
using Continuation = const OperationBase*;
using Continuations = std::vector<Continuation>;

class OperationBase {
public:
    using TypeId = const void*;

    virtual ~OperationBase() = default;

    static Operation
    create(const std::vector<grammar::Instruction>& instructions,
           const std::vector<FunctionType>& func_types, Operations& targets,
           std::pmr::polymorphic_allocator<std::byte>& arena);

    virtual Continuation action(Instance&) { return next_; }
    virtual Expected<Continuation> eval(Context&) const {
        return Unexpected(ERROR("evaluate called on non constant expression"));
    }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }
    template <class Derived> Derived& as() {
        return reinterpret_cast<Derived&>(*this);
    }
    template <class Derived> bool is() {
        return type() == Derived::static_type();
    }

    virtual TypeId type() const { return nullptr; }

    virtual std::string getName() { return "?"; }

    size_t getAddress() { return addr_; }
    std::string getFormattedAddress(Instance& instance) const;

protected:
    class CollectorBase {
    public:
        CollectorBase(const std::vector<grammar::Instruction>& instructions)
            : instructions_(instructions) {}

        virtual ~CollectorBase() = default;

        virtual Operation finish() { return do_finish(); }

        virtual void add(Operation next);

        const std::vector<grammar::Instruction>& getInstructions() {
            return instructions_;
        }

    protected:
        Operation do_finish();

    private:
        const std::vector<grammar::Instruction>& instructions_;
        Operations operations_;
    };

    using Collector = std::unique_ptr<CollectorBase>;

    OperationBase() = default;
    OperationBase(size_t addr) : addr_(addr) {}

    Continuation trap(Instance& instance, std::string msg, size_t addr) const;

    Operation next_ = nullptr;
    uint32_t addr_ = UINT32_MAX;

private:
    static Operation
    do_create(const grammar::Instruction& inst,
              const std::vector<FunctionType>& func_types, Operations& targets,
              std::pmr::polymorphic_allocator<std::byte>& arena);
};

template <typename Derived> class TaggedOperation : public OperationBase {
public:
    using OperationBase::OperationBase;

    static TypeId static_type() {
        static int id;
        return &id;
    }

    TypeId type() const override { return static_type(); }

    std::string getName() override {
        int status = 0;
        char* demangled = abi::__cxa_demangle(typeid(Derived).name(), nullptr,
                                              nullptr, &status);
        std::string name = (status == 0) ? demangled : typeid(Derived).name();
        std::free(demangled);

        auto pos = name.rfind("::");
        if (pos == std::string_view::npos)
            return std::string(name);

        return std::string(name.substr(pos + 2));
    }
};

template <typename Derived>
class GenericOperation : public TaggedOperation<Derived> {
public:
    using TaggedOperation<Derived>::TaggedOperation;

    Continuation action(Instance& instance) override;
};

/***************************/
/* Parametric Instructions */
/***************************/

class Drop : public TaggedOperation<Drop> {
public:
    Drop(const grammar::Drop& drop);

    Continuation action(Instance& instance) override;
};

class Select : public TaggedOperation<Select> {
public:
    Continuation action(Instance& instance) override;
};

/***********************/
/* Memory Instructions */
/***********************/

class I32Load : public TaggedOperation<I32Load> {
public:
    I32Load(const grammar::I32Load& i32_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load : public TaggedOperation<I64Load> {
public:
    I64Load(const grammar::I64Load& i64_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class F32Load : public OperationBase {
public:
    F32Load(const grammar::F32Load& f32_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class F64Load : public OperationBase {
public:
    F64Load(const grammar::F64Load& f64_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load8Signed : public TaggedOperation<I32Load8Signed> {
public:
    I32Load8Signed(const grammar::I32Load8Signed& i32_load8_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load8Unsigned : public TaggedOperation<I32Load8Unsigned> {
public:
    I32Load8Unsigned(const grammar::I32Load8Unsigned& i32_load8_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load16Signed : public TaggedOperation<I32Load16Signed> {
public:
    I32Load16Signed(const grammar::I32Load16Signed& i32_load16_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Load16Unsigned : public TaggedOperation<I32Load16Unsigned> {
public:
    I32Load16Unsigned(const grammar::I32Load16Unsigned& i32_load16_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load8Signed : public OperationBase {
public:
    I64Load8Signed(const grammar::I64Load8Signed& i64_load8_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load8Unsigned : public OperationBase {
public:
    I64Load8Unsigned(const grammar::I64Load8Unsigned& i64_load8_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load16Signed : public OperationBase {
public:
    I64Load16Signed(const grammar::I64Load16Signed& i64_load16_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load16Unsigned : public OperationBase {
public:
    I64Load16Unsigned(const grammar::I64Load16Unsigned& i64_load16_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load32Signed : public OperationBase {
public:
    I64Load32Signed(const grammar::I64Load32Signed& i64_load32_s);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Load32Unsigned : public OperationBase {
public:
    I64Load32Unsigned(const grammar::I64Load32Unsigned& i64_load32_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Store : public TaggedOperation<I32Store> {
public:
    I32Store(const grammar::I32Store& i32_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Store : public TaggedOperation<I64Store> {
public:
    I64Store(const grammar::I64Store& i64_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class F32Store : public OperationBase {
public:
    F32Store(const grammar::F32Store& f32_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class F64Store : public OperationBase {
public:
    F64Store(const grammar::F64Store& f64_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Store8 : public OperationBase {
public:
    I32Store8(const grammar::I32Store8& i32_store8);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I32Store16 : public OperationBase {
public:
    I32Store16(const grammar::I32Store16& i32_store16);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Store8 : public OperationBase {
public:
    I64Store8(const grammar::I64Store8& i64_store8);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Store16 : public OperationBase {
public:
    I64Store16(const grammar::I64Store16& i64_store16);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class I64Store32 : public OperationBase {
public:
    I64Store32(const grammar::I64Store32& i64_store32);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class MemoryGrow : public OperationBase {
public:
    MemoryGrow(const grammar::MemoryGrow& mem_grow);

    Continuation action(Instance& instance) override;
};

class MemoryInit : public OperationBase {
public:
    MemoryInit(const grammar::MemoryInit& mem_init);

    Continuation action(Instance& instance) override;

private:
    uint32_t segment_idx_;
};

class DataDrop : public OperationBase {
public:
    DataDrop(const grammar::DataDrop& data_drop);

    Continuation action(Instance& instance) override;

private:
    uint32_t segment_idx_;
};

class MemoryCopy : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

class MemoryFill : public OperationBase {
public:
    MemoryFill(const grammar::MemoryFill& mem_fill)
        : OperationBase(mem_fill.getAddress()) {}

    Continuation action(Instance& instance) override;
};

/******************************/
/* Atomic Memory Instructions */
/******************************/

class AtomicNotify : public OperationBase {
public:
    AtomicNotify(const grammar::AtomicNotify& atomic_notify);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicWait32 : public OperationBase {
public:
    AtomicWait32(const grammar::AtomicWait32& atomic_wait32);

    Continuation action(Instance& instance) override;

private:
    Continuation idle() const;

    uint32_t offset_;
    uint32_t align_;
};

class AtomicLoad : public OperationBase {
public:
    AtomicLoad(const grammar::AtomicLoad& atomic_load);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicLoad8Unsigned : public OperationBase {
public:
    AtomicLoad8Unsigned(const grammar::AtomicLoad8Unsigned& atomic_load8_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicStore : public OperationBase {
public:
    AtomicStore(const grammar::AtomicStore& atomic_store);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicStore8 : public OperationBase {
public:
    AtomicStore8(const grammar::AtomicStore8& atomic_store8);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicAdd : public OperationBase {
public:
    AtomicAdd(const grammar::AtomicAdd& atomic_add);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicExchange8Unsigned : public OperationBase {
public:
    AtomicExchange8Unsigned(
        const grammar::AtomicExchange8Unsigned& atomic_xchg8_u);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

class AtomicCompareExchange : public OperationBase {
public:
    AtomicCompareExchange(const grammar::AtomicCompareExchange& atomic_cmpxchg);

    Continuation action(Instance& instance) override;

private:
    uint32_t offset_;
    uint32_t align_;
};

template <typename T, typename... Args>
T* arena_new(std::pmr::polymorphic_allocator<std::byte>& arena,
             Args&&... args) {
    T* ptr = arena.allocate_object<T>();

    try {
        std::construct_at(ptr, std::forward<Args>(args)...);
    } catch (...) {
        arena.deallocate_object(ptr);
        throw;
    }

    return ptr;
}

} // namespace runtime

namespace std::pmr {

template <class T, class... Args>
std::shared_ptr<T> make_shared(std::pmr::polymorphic_allocator<std::byte>& a,
                               Args&&... args) {
    return std::allocate_shared<T>(a, std::forward<Args>(args)...);
}

}; // namespace std::pmr
