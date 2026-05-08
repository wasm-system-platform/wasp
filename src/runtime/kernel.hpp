#pragma once

#include "hw/mem/mmu.hpp"
#include "runtime/instance.hpp"
#include "runtime/process_manager.hpp"

namespace runtime {

using hw::mem::MemoryManagementUnit;

class Kernel : public TaggedInstance<Kernel> {
public:
    static Expected<std::shared_ptr<Kernel>>
    create(const std::string& kernel_path);

    MemoryManagementUnit& getMMU() { return *mmu_; }

    void switchToInstance(Instance& instance) { active_instance_ = &instance; }
    void switchBack() { active_instance_ = this; }

    Continuation callPageFaultHandler();

    bool functionExists(uint32_t idx, size_t signature);

protected:
    Kernel(GlobalState&& global_state, std::shared_ptr<Exports>& exports,
           const grammar::Module& module)
        : TaggedInstance<Kernel>(std::move(global_state), exports, module) {}

private:
    Instance* active_instance_;

    std::shared_ptr<InterruptController> controller_;
    std::unique_ptr<MemoryManagementUnit> mmu_;
    std::unique_ptr<ProcessManager> proccess_manager_;

    static Expected<Imports> createImports();

    Expected<void> validateExports();

    Expected<void> setupInterruptController();
    Expected<void> setupMMU();
    Expected<void> setupDeviceManager();
    Expected<void> setupProcessManager();

    void run(OperationBase& operation) override;
};

} // namespace runtime
