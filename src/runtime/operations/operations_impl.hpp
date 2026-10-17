#pragma once

#include "runtime/instance.hpp"

namespace runtime {

template <typename Derived>
Continuation GenericOperation<Derived>::action(Instance& instance) {
    Stack& stack = instance.getActiveContext().getStack();

    if constexpr (std::is_same_v<typename Derived::InType, void>) {
        if constexpr (std::is_same_v<typename Derived::OutType, void>) {
            Derived::impl(static_cast<Derived&>(*this), instance);
        } else {
            auto out = Derived::impl(static_cast<Derived&>(*this), instance);
            stack.push(out);
        }
    } else if constexpr (std::is_same_v<typename Derived::InType, Value>) {
        Value in = stack.pop();

        if constexpr (std::is_same_v<typename Derived::OutType, void>) {
            Derived::impl(static_cast<Derived&>(*this), instance, in);
        } else {
            auto out =
                Derived::impl(static_cast<Derived&>(*this), instance, in);
            stack.push(out);
        }
    } else {
        auto in = stack.pop<typename Derived::InType>();

        if constexpr (std::is_same_v<typename Derived::OutType, void>) {
            std::apply(
                [&](auto&&... args) {
                    Derived::impl(static_cast<Derived&>(*this), instance,
                                  std::forward<decltype(args)>(args)...);
                },
                in);
        } else {
            auto out = std::apply(
                [&](auto&&... args) {
                    return Derived::impl(static_cast<Derived&>(*this), instance,
                                         std::forward<decltype(args)>(args)...);
                },
                in);
            stack.push(out);
        }
    }

    return this->next_.get();
}

}
