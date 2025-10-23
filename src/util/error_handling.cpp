#include <fmt/format.h>

#include "util/error_handling.hpp"

Error::Error(const std::string& msg, const char* func, const char* file,
             int line)
    : msg_(msg) {
    call_stack_.emplace_back(func, file, line);
}

Error::Error(const Error& inner, const char* func, const char* file, int line)
    : msg_(inner.msg_), call_stack_(inner.call_stack_) {
    call_stack_.emplace_back(func, file, line);
}

std::string Error::toString() const {
    std::string s = fmt::format("Error: {}\n", msg_);
    for (int i = static_cast<int>(call_stack_.size()) - 1; i >= 0; --i) {
        const auto& [func, file, line] = call_stack_[i];
        s += fmt::format("  at {} ({}:{})\n", func, file, line);
    }
    return s;
}