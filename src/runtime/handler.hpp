#pragma once

#include "runtime/operations.hpp"

namespace runtime {

class Handler : public OperationBase {
public:
    Handler(uint32_t handler_idx) : handler_idx_(handler_idx) {}

protected:
    class Epilogue : public OperationBase {
    protected:
        Epilogue(Instance& suspended_instance)
            : suspended_instance_(suspended_instance) {}

        Instance& suspended_instance_;
    };

    uint32_t handler_idx_;
    std::vector<Operation> epilogues_;
};

/*********************/
/* Interrupt Handler */
/*********************/

class InterruptController;

class Interrupt : public Handler {
public:
    Interrupt(InterruptController& controller, uint32_t interrupt_handler_idx)
        : Handler(interrupt_handler_idx), controller_(controller) {}

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public Handler::Epilogue {
    public:
        Epilogue(Instance& suspended_instance, Interrupt& parent)
            : Handler::Epilogue(suspended_instance), parent_(parent) {}

        Continuation action(Instance& instance) override;

    private:
        Interrupt& parent_;
    };

    InterruptController& controller_;
};

/*******************
 * SysCall Handler *
 *******************/

class SysCall : public Handler {
public:
    SysCall(uint32_t sys_call_handler_idx) : Handler(sys_call_handler_idx) {}

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public Handler::Epilogue {
    public:
        Epilogue(Instance& suspended_instance, SysCall& parent)
            : Handler::Epilogue(suspended_instance), parent_(parent) {}

        Continuation action(Instance& instance) override;

    private:
        SysCall& parent_;
    };
};

/**********************
 * Page Fault Handler *
 **********************/

class PageFault : public Handler {
public:
    PageFault(uint32_t page_fault_handler_idx)
        : Handler(page_fault_handler_idx) {}

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public Handler::Epilogue {
    public:
        Epilogue(Instance& suspended_instance, PageFault& parent)
            : Handler::Epilogue(suspended_instance), parent_(parent) {}

        Continuation action(Instance& instance) override;

    private:
        PageFault& parent_;
    };
};

} // namespace runtime
