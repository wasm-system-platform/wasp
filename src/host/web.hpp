#pragma once

#include <cstdint>

namespace host {

template<class Impl> class BasicArch;

class WebArch {
private:
    static constexpr double TARGET_INTERVAL_MS = 15.0;

    WebArch();

    void tick();

    template <typename Atomic, typename T>
    void wait(Atomic& atomic, T old_value);

    friend class BasicArch<WebArch>;

    uint64_t ticks_since_check_ = 0;
    uint64_t ticks_until_check_ = 0;
    double last_yield_time_ms_;
};

}
