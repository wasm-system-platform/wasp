#pragma once

#include <iostream>
#include <vector>

#include "devices/timer.hpp"
#include "runtime/context.hpp"
#include "runtime/vm.hpp"
#include "runtime/waiting_room.hpp"

namespace runtime {

/*******************/
/* Module Instance */
/*******************/

class Instance : public std::enable_shared_from_this<Instance> {
public:
    struct Export {
        uint32_t func_idx;
        FunctionType func_type;
        size_t signature;
    };

    static constexpr size_t DEFAULT_CONTEXT = 0;

    static Expected<std::shared_ptr<Instance>>
    create(const grammar::Module& module,
           const std::unordered_map<std::string, Function>& imports);

    static Expected<std::shared_ptr<Instance>>
    createKernel(const std::string& kernel_path,
                 const std::string& rootfs_path);
    static Expected<std::shared_ptr<Instance>>
    createUserProgram(const std::string& program, Instance& parent);

    Expected<void> call(const std::string& name);
    Expected<void> call(const std::string& name, int32_t a);
    Expected<void> call(const std::string& name, int32_t a, int32_t b);

    Expected<int32_t> callRet(const std::string& name, int32_t a, int32_t b);

    inline Context& getActiveContext() { return *active_context_; };
    inline void setContext(const std::shared_ptr<Context>& context) {
        active_context_ = context.get();
    }
    inline GlobalState& getGlobalState() { return state_; }

    inline Continuation tick() { return state_.tick(*this, devices_); }

    inline WaitingRoom& getWaitingRoom() { return WaitingRoom::instance(); }
    inline Timer& getTimer() { return devices_[TIMER_PORT]->as<Timer>(); }

    inline Instance& getKernel() {
        if (parent_)
            return parent_->getKernel();
        return *this;
    }
    inline void switchToKernel() {
        Instance& kernel = getKernel();
        kernel.active_instance_ = &kernel;
    }
    inline void switchToInstance(Instance& instance) {
        Instance& kernel = getKernel();
        kernel.active_instance_ = &instance;
    }

private:
    class SysCall : public OperationBase {
    public:
        SysCall(uint32_t sys_call_handler_idx);

        Continuation action(Instance& instance) override;

    private:
        class Epilogue : public OperationBase {
        public:
            Epilogue(size_t id, Instance& suspended_instance, SysCall& parent);

            Continuation action(Instance& instance) override;

        private:
            size_t id_;
            Instance& suspended_instance_;
            SysCall& parent_;
        };

        uint32_t sys_call_handler_idx_;

        std::vector<Operation> epilogues_;
        std::stack<size_t> free_list_;

        const Operation& createEpilogue(Instance& instance);
        void destroyEpilogue(size_t id);
    };

    Instance* parent_ = nullptr;

    Instance* active_instance_;
    GlobalState state_;
    Context* active_context_;

    std::unordered_map<std::string, Export> exports_;
    std::unordered_map<uint32_t, Device> devices_;

    Instance(GlobalState&& store,
             std::unordered_map<std::string, Export>&& exports,
             const grammar::Module& module);

    Expected<void> call(const std::string& name, const FunctionType& func_type);

    void invoke(size_t func_idx);
    void invokeIndirect(uint32_t element_idx, size_t signature);

    void run(OperationBase& operation);
};

} // namespace runtime
