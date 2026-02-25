#pragma once

#include "runtime/context.hpp"
#include "runtime/vm.hpp"
#include "runtime/waiting_room.hpp"

namespace runtime {

/*******************/
/* Module Instance */
/*******************/

class Kernel;
using Imports = std::unordered_map<std::string, Function>;

class Instance : public std::enable_shared_from_this<Instance> {
public:
    struct Export {
        uint32_t func_idx;
        FunctionType func_type;
        size_t signature;
    };
    using Exports = std::unordered_map<std::string, Export>;

    static Expected<std::shared_ptr<Instance>>
    create(const grammar::Module& module,
           const std::unordered_map<std::string, Function>& imports);

    virtual ~Instance() = default;

    Expected<void> call(const std::string& name);
    Expected<void> call(const std::string& name, int32_t a);
    Expected<void> call(const std::string& name, int32_t a, int32_t b);

    Expected<int32_t> callRet(const std::string& name, int32_t a, int32_t b);

    inline Context& getActiveContext() {
        assert(active_context_);
        return *active_context_;
    }
    inline void setActiveContext(Context& context) {
        active_context_ = &context;
    }

    inline GlobalState& getGlobalState() { return global_state_; }
    inline WaitingRoom& getWaitingRoom() { return WaitingRoom::instance(); }

    template <class Derived> const Derived& as() const {
        return reinterpret_cast<const Derived&>(*this);
    }
    template <class Derived> Derived& as() {
        return reinterpret_cast<Derived&>(*this);
    }
    template <class Derived> bool is() {
        return type() == Derived::static_type();
    }

protected:
    using TypeId = const void*;

    std::shared_ptr<Exports> exports_;

    static Expected<std::shared_ptr<Exports>>
    createExports(const grammar::Module& module);

    Instance(GlobalState&& global_state,
             std::shared_ptr<Exports>& exports,
             const grammar::Module& module);

private:
    GlobalState global_state_;
    Context* active_context_ = nullptr;

    Expected<void> call(const std::string& name, const FunctionType& func_type);

    void invoke(size_t func_idx);
    void invokeIndirect(uint32_t element_idx, size_t signature);
    
    virtual void run(OperationBase& entry);

    virtual TypeId type() const {
        return nullptr;
    }

    friend class Process;
};

template<typename Derived>
class TaggedInstance : public Instance {
public:
    using Instance::Instance;

    static TypeId static_type() {
        static int id;
        return &id;
    }

    TypeId type() const override {
        return static_type();
    }
};

} // namespace runtime
