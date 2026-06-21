#include <fstream>

#include "fmt/format.h"
#include "grammar/values.hpp"

Expected<S32> S32::parse(ByteCursor& in) {
    uint32_t result = 0;
    int shift = 0;

    for (int i = 0; i < 5; i++) {
        uint8_t byte = in.byte();
        result |= static_cast<uint32_t>(byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            if (in.bad())
                return Unexpected(ERROR("broken reader"));

            if ((byte & 0x40) != 0 && shift < 32)
                result |= (~uint32_t{0}) << shift;

            return S32(static_cast<int32_t>(result));
        }
    }

    return Unexpected(ERROR("invalid s32 LEB128"));
}

Expected<U32> U32::parse(ByteCursor& in) {
    uint32_t result = 0;
    int shift = 0;

    for (int i = 0; i < 5; i++) {
        uint8_t byte = in.byte();
        result |= static_cast<uint32_t>(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0) {
            if (in.bad())
                return Unexpected(ERROR("broken reader"));

            return U32(result);
        }

        shift += 7;
    }

    return Unexpected(ERROR("invalid u32 LEB128"));
}

Expected<S64> S64::parse(ByteCursor& in) {
    uint64_t result = 0;
    int shift = 0;

    for (int i = 0; i < 10; i++) {
        uint8_t byte = in.byte();
        result |= static_cast<uint64_t>(byte & 0x7F) << shift;
        shift += 7;

        if ((byte & 0x80) == 0) {
            if (in.bad())
                return Unexpected(ERROR("broken reader"));

            if ((byte & 0x40) != 0 && shift < 32)
                result |= (~uint64_t{0}) << shift;

            return S64(static_cast<int32_t>(result));
        }
    }

    return Unexpected(ERROR("invalid s64 LEB128"));
}

Expected<U64> U64::parse(ByteCursor& in) {
    uint64_t result = 0;
    int shift = 0;

    for (int i = 0; i < 10; i++) {
        uint8_t byte = in.byte();
        result |= static_cast<uint64_t>(byte & 0x7F) << shift;

        if ((byte & 0x80) == 0) {
            if (in.bad())
                return Unexpected(ERROR("broken reader"));

            return U64(result);
        }

        shift += 7;
    }

    return Unexpected(ERROR("invalid u64 LEB128"));
}

/******************/
/* Floating-Point */
/******************/

Expected<F32> F32::parse(ByteCursor& in) {
    float f = in.f32();

    if (in.bad())
        return Unexpected(ERROR("invalid f32"));

    return F32(f);
}

Expected<F64> F64::parse(ByteCursor& in) {
    double f = in.f64();

    if (in.bad())
        return Unexpected(ERROR("invalid f64"));

    return F64(f);
}

/*********/
/* Names */
/*********/

Expected<Name> Name::parse(ByteCursor& in) {
    Expected<U32> u32_exp = U32::parse(in);
    if (!u32_exp.has_value())
        return Unexpected(PROPAGATE(u32_exp));

    uint32_t len = *u32_exp;
    std::span<const uint8_t> bytes = in.read(len);

    if (in.bad())
        return Unexpected(ERROR("invalid name"));

    return Name(std::string_view(reinterpret_cast<const char*>(bytes.data()),
                                 bytes.size()));
}
