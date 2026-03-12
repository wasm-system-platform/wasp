#pragma once

#include <vector>

#include "runtime/context.hpp"
#include "runtime/operations.hpp"

namespace runtime {

class Instance;

class Function {
public:
    static constexpr uint32_t NULL_IDX = UINT32_MAX;

    Function(Operation&& body, size_t num_params, std::vector<Value>&& locals,
             size_t signature, std::string formatted_type = "");

    static Function create(const FunctionType& type,
                           const std::vector<FunctionType>& types,
                           const grammar::Function& func);

    // () -> ()
    static Function createExternal(std::function<void(Instance&)>);
    // (i32) -> ()
    static Function createExternal(std::function<void(Instance&, int32_t)>);
    // (i32, i32) -> ()
    static Function
        createExternal(std::function<void(Instance&, int32_t, int32_t)>);
    // (i32, i32, i32) -> ()
    static Function createExternal(
        std::function<void(Instance&, int32_t, int32_t, int32_t)>);
    // () -> (i32)
    static Function createExternal(std::function<int32_t(Instance&)>);
    // () -> (i64)
    static Function createExternal(std::function<int64_t(Instance&)>);
    // (i32) -> (i32)
    static Function createExternal(std::function<int32_t(Instance&, int32_t)>);
    // (i32) -> (i32, i32)
    static Function
        createExternal(std::function<int32_t(Instance&, int32_t, int32_t)>);
    // (i32) -> (i32, i32)
    static Function createExternal(
        std::function<int32_t(Instance&, int32_t, int32_t, int32_t)>);
    // (i32) -> (i32, i32, i32, i32)
    static Function createExternal(
        std::function<int32_t(Instance&, int32_t, int32_t, int32_t, int32_t)>);

    inline Continuation enterFrame(Context& context) {
        TRACE_VERBOSE(" --- enter frame ---");
        for (int i = num_params_ - 1; i >= 0; --i) {
            locals_[i] = context.pop();
        }
        context.pushLocals(locals_);

        return body_.get();
    }

    inline void leaveFrame(Context& context) {
        TRACE_VERBOSE(" --- leave frame ---");
        context.popLocals();
    }

    inline size_t getSignature() const { return signature_; }

    const std::string& getFormattedType() const { return formatted_type_; }

    bool isCompatible(const FunctionType& other) const;

private:
    class Wrapper : public OperationBase {
    public:
        Wrapper(std::function<void(Instance& instance)>&& func);

        Continuation action(Instance& instance) override;

    private:
        std::function<void(Instance& instance)> func_;
    };

    Operation body_;
    size_t num_params_;
    std::vector<Value> locals_;
    size_t signature_;
    std::string formatted_type_;
};

} // namespace runtime
