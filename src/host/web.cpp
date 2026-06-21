#include "fmt/base.h"
#include <atomic>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "host/web.hpp"

using namespace host;

WebArch::WebArch() {
#ifdef __EMSCRIPTEN__
    last_yield_time_ms_ = emscripten_get_now();
#endif
}

void WebArch::tick() {
#ifdef __EMSCRIPTEN__
    ticks_since_check_ += 1;

    if (ticks_since_check_ < ticks_until_check_)
        return;

    ticks_since_check_ = 0;

    const double now = emscripten_get_now();
    const double elapsed_ms = now - last_yield_time_ms_;

    if (elapsed_ms < TARGET_INTERVAL_MS) {
        ticks_until_check_ += 1;
        return;
    }

    /* Yield to the browser. */
    ticks_until_check_ /= 2;
    emscripten_sleep(0);
    last_yield_time_ms_ = emscripten_get_now();
#endif
}

template <>
void WebArch::wait<std::atomic<uint32_t>, uint32_t>(std::atomic<uint32_t>&,
                                                    uint32_t) {
#ifdef __EMSCRIPTEN__
    /* Yield to the browser. */
    emscripten_sleep(1);
#endif
}
