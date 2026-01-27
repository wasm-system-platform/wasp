#pragma once

#include "runtime/operations.hpp"

namespace runtime {

class Handler : public OperationBase {
public:
    Handler(uint32_t handler_idx) : handler_idx_(handler_idx) {}

protected:
    uint32_t handler_idx_;
    std::vector<Operation> epilogues_;

    size_t allocateEpilogue(Instance& instance);
    void freeEpilogue(size_t id);

private:
    
    std::stack<size_t> free_list_;
};

/*******************
 * SysCall Handler *
 *******************/

class SysCall : public Handler {
public:
    SysCall(uint32_t sys_call_handler_idx) : Handler(sys_call_handler_idx) {}

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public OperationBase {
    public:
        Epilogue(size_t id, Instance& suspended_instance, SysCall& parent) : id_(id), suspended_instance_(suspended_instance), parent_(parent) {}

        Continuation action(Instance& instance) override;

    private:
        size_t id_;
        Instance& suspended_instance_;
        SysCall& parent_;
    };
};

/**********************
 * Page Fault Handler *
 **********************/

class PageFault : public Handler {
public:
    PageFault(uint32_t page_fault_handler_idx) : Handler(page_fault_handler_idx) {}

    Continuation action(Instance& instance) override;

private:
    class Epilogue : public OperationBase {
    public:
        Epilogue(size_t id, Instance& suspended_instance, PageFault& parent) : id_(id), suspended_instance_(suspended_instance), parent_(parent) {}

        Continuation action(Instance& instance) override;

    private:
        size_t id_;
        Instance& suspended_instance_;
        PageFault& parent_;
    };
};

} // namespace runtime

