#pragma once

#include "host/web.hpp"

namespace host {

template <typename Impl> class BasicArch {
public:
    static BasicArch& instance() {
        static BasicArch arch;
        return arch;
    }

    void tick() {
        impl_.tick();
    }

    template <typename Atomic, typename T>
    void wait(Atomic& atomic, T old_value) {
        impl_.wait(atomic, old_value);
    }

private:
    Impl impl_;
};

class GenericArch {
protected:
    void tick() {}

    template <typename Atomic, typename T>
    void wait(Atomic& atomic, T old_value) {
        atomic.wait(old_value);
    }
};

#if defined(__EMSCRIPTEN__)
using Arch = BasicArch<WebArch>;
#else
using Arch = BasicArch<GenericArch>;
#endif

}