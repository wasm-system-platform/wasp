#pragma once

#include "runtime/operations.hpp"

namespace runtime {

/*************/
/* Sigsetjmp */
/*************/

class Sigsetjmp : public OperationBase {
public:
    Continuation action(Instance& instance) override;

    void setPrologueFuncIdx(uint32_t prologue_func_idx) { prologue_call_ = std::make_shared<Call>(prologue_func_idx, addr_); }
    void setEpilogueFuncIdx(uint32_t epilogue_func_idx) { epilogue_->as<Epilogue>().setEpilogueFuncIdx(epilogue_func_idx); }

private:
    class Epilogue : public OperationBase {
    public:
        Continuation action(Instance& instance) override;  

        void setEpilogueFuncIdx(uint32_t epilogue_func_idx) { epilogue_call_ = std::make_shared<Call>(epilogue_func_idx, addr_); }

    private:
        Operation epilogue_call_;
    };

    Operation prologue_call_;
    Operation epilogue_ = std::make_shared<Epilogue>();
};

/***********/
/* Longjmp */
/***********/

class Longjmp : public OperationBase {
public:
    Continuation action(Instance& instance) override;
};

}