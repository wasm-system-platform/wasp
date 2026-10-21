#pragma once

#include "runtime/operations/operations.hpp"

namespace runtime {

class Handler : public OperationBase {
public:
    Handler(uint32_t handler_idx) : handler_idx_(handler_idx) {}

protected:
    class Epilogue : public OperationBase {
    public:
        void suspandInstance(Instance& instance) {
            suspended_instances_.push(instance);
        }

        Instance& resumeInstance() {
            Instance& instance = suspended_instances_.top().get();
            suspended_instances_.pop();
            return instance;
        }

    private:
        std::stack<std::reference_wrapper<Instance>> suspended_instances_;
    };

    uint32_t handler_idx_;
};

/*********************/
/* Interrupt Handler */
/*********************/

class InterruptController;

class Interrupt : public Handler {
public:
    Interrupt(InterruptController& controller, uint32_t interrupt_handler_idx)
        : Handler(interrupt_handler_idx), epilogue_(*this),
          controller_(controller) {}

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public Handler::Epilogue {
    public:
        Epilogue(Interrupt& parent) : parent_(parent) {}

        Continuation action(Instance& instance) override;

    private:
        Interrupt& parent_;
    };

    Epilogue epilogue_;
    InterruptController& controller_;
};

/*******************
 * SysCall Handler *
 *******************/

class SysCall : public Handler {
public:
    SysCall(uint32_t sys_call_handler_idx)
        : Handler(sys_call_handler_idx), epilogue_(*this) {}

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public Handler::Epilogue {
    public:
        Epilogue(SysCall& parent) : parent_(parent) {}

        Continuation action(Instance& instance) override;

    private:
        SysCall& parent_;
    };

    Epilogue epilogue_;
};

/**********************
 * Page Fault Handler *
 **********************/

class PageFault : public Handler {
public:
    PageFault(uint32_t page_fault_handler_idx)
        : Handler(page_fault_handler_idx), epilogue_(*this) {}

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public Handler::Epilogue {
    public:
        Epilogue(PageFault& parent) : parent_(parent) {}

        Continuation action(Instance& instance) override;

    private:
        PageFault& parent_;
    };

    Epilogue epilogue_;
};

/******************
 * Signal Handler *
 ******************/

class Signal : public Handler {
public:
    Signal() : Handler(static_cast<uint32_t>(-1)) {}

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public Handler::Epilogue {
    public:
        Continuation action(Instance& instance) override;
    };

    Epilogue epilogue_;
};

} // namespace runtime
