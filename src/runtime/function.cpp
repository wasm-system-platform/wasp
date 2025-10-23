#include "runtime/function.hpp"
#include "runtime/instance.hpp"
#include "runtime/operations.hpp"

namespace runtime {

Function::Function(Operation&& body, size_t num_params,
                   std::vector<Value>&& locals, size_t signature)
    : body_(body), num_params_(num_params), locals_(std::move(locals)),
      signature_(signature) {}

Function Function::create(const FunctionType& type,
                          const std::vector<FunctionType>& types,
                          const grammar::Function& func) {
    Operations targets;
    Operation body =
        OperationBase::create(func.getBody().getInstructions(), types, targets);
    size_t num_params = type.getParamTypes().getValueTypes().size();

    std::vector<Value> locals(num_params);
    for (const auto& local : func.getLocals()) {
        if (local.getType() == ValueType::Type::i32)
            locals.push_back(int32_t(0));
        else if (local.getType() == ValueType::Type::i64)
            locals.push_back(int64_t(0));
        else if (local.getType() == ValueType::Type::f32)
            locals.push_back(float(0));
        else
            locals.push_back(double(0));
    };

    size_t signature = std::hash<FunctionType>()(type);
    return Function(std::move(body), num_params, std::move(locals), signature);
}

/*******************/
/* Create External */
/*******************/

// () -> ()
Function
Function::createExternal(std::function<void(Instance&)> external_func) {
    Operation body = std::make_shared<Wrapper>(
        [external_func](Instance& instance) { external_func(instance); });

    size_t signature = std::hash<FunctionType>()(FunctionType::Empty());
    return Function(std::move(body), 0, {}, signature);
}

// () -> (i32)
Function Function::createExternal(
    std::function<void(Instance&, int32_t)> external_func) {
    Operation body =
        std::make_shared<Wrapper>([external_func](Instance& instance) {
            int32_t a =
                std::get<int32_t>(instance.getActiveContext().getLocal(0));
            external_func(instance, a);
        });

    size_t signature = std::hash<FunctionType>()(FunctionType::ConsumerI32());
    return Function(std::move(body), 1, {int32_t(0)}, signature);
}

// () -> (i32, i32)
Function Function::createExternal(
    std::function<void(Instance&, int32_t, int32_t)> external_func) {
    Operation body =
        std::make_shared<Wrapper>([external_func](Instance& instance) {
            Context& context = instance.getActiveContext();
            int32_t a = std::get<int32_t>(context.getLocal(0));
            int32_t b = std::get<int32_t>(context.getLocal(1));
            external_func(instance, a, b);
        });

    size_t signature =
        std::hash<FunctionType>()(FunctionType::ConsumerI32I32());
    return Function(std::move(body), 2, {int32_t(0), int32_t(0)}, signature);
}

// () -> (i32, i32, i32)
Function Function::createExternal(
    std::function<void(Instance&, int32_t, int32_t, int32_t)> external_func) {
    Operation body =
        std::make_shared<Wrapper>([external_func](Instance& instance) {
            Context& context = instance.getActiveContext();
            int32_t a = std::get<int32_t>(context.getLocal(0));
            int32_t b = std::get<int32_t>(context.getLocal(1));
            int32_t c = std::get<int32_t>(context.getLocal(2));
            external_func(instance, a, b, c);
        });

    size_t signature =
        std::hash<FunctionType>()(FunctionType::ConsumerI32I32I32());
    return Function(std::move(body), 3, {int32_t(0), int32_t(0), int32_t(0)},
                    signature);
}

// (i32) -> ()
Function
Function::createExternal(std::function<int32_t(Instance&)> external_func) {
    Operation body =
        std::make_shared<Wrapper>([external_func](Instance& instance) {
            int32_t result = external_func(instance);
            instance.getActiveContext().pushI32(result);
        });

    size_t signature = std::hash<FunctionType>()(FunctionType::Producer());
    return Function(std::move(body), 0, {}, signature);
}

// (i32) -> (i32)
Function Function::createExternal(
    std::function<int32_t(Instance&, int32_t)> external_func) {
    Operation body =
        std::make_shared<Wrapper>([external_func](Instance& instance) {
            Context& context = instance.getActiveContext();
            int32_t a = std::get<int32_t>(context.getLocal(0));

            int32_t result = external_func(instance, a);
            context.pushI32(result);
        });

    size_t signature = std::hash<FunctionType>()(FunctionType::ProducerI32());
    return Function(std::move(body), 1, {int32_t(0)}, signature);
}

// (i32) -> (i32, i32)
Function Function::createExternal(
    std::function<int32_t(Instance&, int32_t, int32_t)> external_func) {
    Operation body =
        std::make_shared<Wrapper>([external_func](Instance& instance) {
            Context& context = instance.getActiveContext();
            int32_t a = std::get<int32_t>(context.getLocal(0));
            int32_t b = std::get<int32_t>(context.getLocal(1));

            int32_t result = external_func(instance, a, b);
            context.pushI32(result);
        });

    size_t signature =
        std::hash<FunctionType>()(FunctionType::ProducerI32I32());
    return Function(std::move(body), 2, {int32_t(0), int32_t(0)}, signature);
}

bool Function::isCompatible(const FunctionType& other) const {
    return signature_ == std::hash<FunctionType>()(other);
}

/********************/
/* Function Wrapper */
/********************/

Function::Wrapper::Wrapper(std::function<void(Instance& instance)>&& func)
    : func_(std::move(func)) {}

Continuation Function::Wrapper::action(Instance& instance) {
    func_(instance);
    return nullptr;
}

} // namespace runtime
