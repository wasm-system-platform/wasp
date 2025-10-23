#include <fstream>

#include "fmt/format.h"
#include "grammar/values.hpp"

Expected<Byte> Byte::parse(std::istream& in) {
    uint8_t b;

    if (!in.read(reinterpret_cast<char*>(&b), 1)) {
        return Unexpected(ERROR("unexpected end of file"));
    }

    return Byte(b);
}

Expected<U32> U32::parse(std::istream& in) {
    uint32_t result = 0;
    int shift = 0;
    uint8_t byte;

    do {
        if (shift >= 32)
            return Unexpected(ERROR("integer too large"));

        if (!in.read(reinterpret_cast<char*>(&byte), 1))
            return Unexpected(ERROR("unexpected end of file"));

        result |= static_cast<uint32_t>(byte & 0x7F) << shift;
        shift += 7;
    } while (byte & 0x80);

    return U32(result);
}

Expected<S32> S32::parse(std::istream& in) {
    int32_t result = 0;
    int shift = 0;
    uint8_t byte;

    do {
        if (shift >= 32)
            return Unexpected(ERROR("integer too large"));

        if (!in.read(reinterpret_cast<char*>(&byte), 1))
            return Unexpected(ERROR("unexpected end of file"));

        result |= static_cast<int32_t>(byte & 0x7F) << shift;
        shift += 7;

    } while (byte & 0x80);

    if ((shift < 32) && ((byte & 0x40) != 0))
        result |= (~0 << shift);

    return S32(result);
}

Expected<S64> S64::parse(std::istream& in) {
    int64_t result = 0;
    int shift = 0;
    uint8_t byte;

    do {
        if (!in.read(reinterpret_cast<char*>(&byte), 1)) {
            return Unexpected(ERROR("unexpected end of file"));
        }

        result |= static_cast<int64_t>(byte & 0x7F) << shift;
        shift += 7;

        if (shift >= 64) {
            return Unexpected(ERROR("integer too large"));
        }

    } while (byte & 0x80);

    if ((shift < 64) && ((byte & 0x40) != 0))
        result |= (~0 << shift);

    return S64(result);
}

Expected<Name> Name::parse(std::istream& in) {
    Expected<U32> u32_exp = U32::parse(in);
    if (!u32_exp.has_value())
        return Unexpected(PROPAGATE(u32_exp));

    uint32_t len = *u32_exp;
    std::string val(len, '\0');

    if (!in.read(val.data(), len)) {
        return Unexpected(ERROR("unexpected end of file"));
    }

    return Name(std::move(val));
}
