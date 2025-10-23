#pragma once

#include <expected>
#include <string>
#include <vector>

#include <fmt/format.h>

#ifdef ENABLE_TRACE
#define TRACE_LOG(...)                                                         \
    do {                                                                       \
        fmt::println(__VA_ARGS__);                                             \
    } while (0)
#else
#define TRACE_LOG(...)                                                         \
    do {                                                                       \
    } while (0)
#endif

#define ERROR(msg) Error(msg, __PRETTY_FUNCTION__, __FILE__, __LINE__)
#define PROPAGATE(err)                                                         \
    Error(err.error(), __PRETTY_FUNCTION__, __FILE__, __LINE__)

class Error {
public:
    Error(const std::string& msg, const char* func, const char* file, int line);
    Error(const Error& inner, const char* func, const char* file, int line);

    std::string toString() const;

private:
    std::string msg_;
    std::vector<std::tuple<const char*, const char*, int>> call_stack_;
};

template <class T> using Expected = std::expected<T, Error>;

using Unexpected = std::unexpected<Error>;
