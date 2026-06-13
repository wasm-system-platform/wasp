#include <fmt/format.h>
#include <fmt/ranges.h>
#include <unordered_map>

#include "grammar/types.hpp"
#include "grammar/values.hpp"

/******************/
/* Reference Type */
/******************/

Expected<ReferenceType> ReferenceType::parse(std::istream& in) {
    Expected<Byte> type_exp = Byte::parse(in);
    if (!type_exp)
        return Unexpected(PROPAGATE(type_exp));
    uint8_t type = *type_exp;

    switch (type) {
    case static_cast<uint8_t>(Type::funcref):
        return ReferenceType(Type::funcref);
    case static_cast<uint8_t>(Type::externref):
        return ReferenceType(Type::externref);
    default:
        return Unexpected(
            ERROR(fmt::format("unexpected reference type: 0x{:X}", type)));
    }
}

std::string ReferenceType::toString() const {
    return fmt::format("type={}",
                       type_ == Type::funcref ? "funcref" : "externref");
}

ReferenceType::ReferenceType(Type type) : type_(type) {}

/**************/
/* Value Type */
/**************/

Expected<ValueType> ValueType::parse(std::istream& in) {
    Expected<Byte> type_exp = Byte::parse(in);
    if (!type_exp)
        return Unexpected(PROPAGATE(type_exp));
    uint8_t type = *type_exp;

    switch (type) {
    case 0x7F:
        return Type::i32;
    case 0x7E:
        return Type::i64;
    case 0x7D:
        return Type::f32;
    case 0x7C:
        return Type::f64;
    case 0x7B:
        return Type::v128;
    case 0x70:
        return Type::funcref;
    case 0x6F:
        return Type::externref;
    default:
        break;
    }

    return Unexpected(ERROR(fmt::format("unexpected type id: {}", type)));
}

std::string ValueType::toString() const {
    static const std::unordered_map<Type, std::string> TYPE_MAP = {
        {Type::i32, "i32"},
        {Type::i64, "i64"},
        {Type::f32, "f32"},
        {Type::f64, "f64"},
        {Type::v128, "v128"},
        {Type::funcref, "funcref"},
        {Type::externref, "externref"},
    };

    return TYPE_MAP.at(type_);
}

Expected<ExpectedType> ExpectedType::parse(std::istream& in) {
    Expected<U32> len_exp = U32::parse(in);
    if (!len_exp)
        return Unexpected(PROPAGATE(len_exp));
    uint32_t len = *len_exp;

    std::vector<ValueType> value_types;
    value_types.reserve(len);

    for (uint32_t i = 0; i < len; i++) {
        Expected<ValueType> vt_exp = ValueType::parse(in);
        if (!vt_exp)
            return Unexpected(PROPAGATE(vt_exp));

        value_types.push_back(*vt_exp);
    }

    return value_types;
}

std::string ExpectedType::toString() const {
    std::vector<std::string> fmt_vts;
    fmt_vts.reserve(value_types_.size());
    for (const auto& vt : value_types_)
        fmt_vts.push_back(vt.toString());

    return fmt::format("({})", fmt::join(fmt_vts, ", "));
}

const FunctionType& FunctionType::Empty() {
    static const FunctionType TYPE(ExpectedType({}), ExpectedType({}));
    return TYPE;
}

const FunctionType& FunctionType::ConsumerI32() {
    static const FunctionType TYPE(ExpectedType({ValueType::Type::i32}),
                                   ExpectedType({}));

    return TYPE;
}

const FunctionType& FunctionType::ConsumerI32x2() {
    static const FunctionType TYPE(
        ExpectedType({ValueType::Type::i32, ValueType::Type::i32}),
        ExpectedType({}));

    return TYPE;
}

const FunctionType& FunctionType::ConsumerI32x3() {
    static const FunctionType TYPE(
        ExpectedType(
            {ValueType::Type::i32, ValueType::Type::i32, ValueType::Type::i32}),
        ExpectedType({}));

    return TYPE;
}

const FunctionType& FunctionType::Producer() {
    static const FunctionType TYPE(ExpectedType({}),
                                   ExpectedType({ValueType::Type::i32}));

    return TYPE;
}

const FunctionType& FunctionType::I64Producer() {
    static const FunctionType TYPE(ExpectedType({}),
                                   ExpectedType({ValueType::Type::i64}));

    return TYPE;
}

const FunctionType& FunctionType::ProducerI32() {
    static const FunctionType TYPE(ExpectedType({ValueType::Type::i32}),
                                   ExpectedType({ValueType::Type::i32}));

    return TYPE;
}

const FunctionType& FunctionType::ProducerI32x2() {
    static const FunctionType TYPE(
        ExpectedType({ValueType::Type::i32, ValueType::Type::i32}),
        ExpectedType({ValueType::Type::i32}));

    return TYPE;
}

const FunctionType& FunctionType::I32Producer_I32_I32_I32() {
    static const FunctionType TYPE(
        ExpectedType(
            {ValueType::Type::i32, ValueType::Type::i32, ValueType::Type::i32}),
        ExpectedType({ValueType::Type::i32}));

    return TYPE;
}

const FunctionType& FunctionType::ProducerI32_I32_I32_I32() {
    static const FunctionType TYPE(
        ExpectedType({ValueType::Type::i32, ValueType::Type::i32,
                      ValueType::Type::i32, ValueType::Type::i32}),
        ExpectedType({ValueType::Type::i32}));

    return TYPE;
}

const FunctionType& FunctionType::ProducerI32_I32_I32_I32_I32_I32() {
    static const FunctionType TYPE(
        ExpectedType({ValueType::Type::i32, ValueType::Type::i32,
                      ValueType::Type::i32, ValueType::Type::i32,
                      ValueType::Type::i32, ValueType::Type::i32}),
        ExpectedType({ValueType::Type::i32}));

    return TYPE;
}

const FunctionType& FunctionType::ProducerI32x7() {
    static const FunctionType TYPE(
        ExpectedType({ValueType::Type::i32, ValueType::Type::i32,
                      ValueType::Type::i32, ValueType::Type::i32,
                      ValueType::Type::i32, ValueType::Type::i32,
                      ValueType::Type::i32}),
        ExpectedType({ValueType::Type::i32}));

    return TYPE;
}

const FunctionType& FunctionType::ProducerI32x8() {
    static const FunctionType TYPE(
        ExpectedType({ValueType::Type::i32, ValueType::Type::i32,
                      ValueType::Type::i32, ValueType::Type::i32,
                      ValueType::Type::i32, ValueType::Type::i32,
                      ValueType::Type::i32, ValueType::Type::i32}),
        ExpectedType({ValueType::Type::i32}));

    return TYPE;
}

Expected<FunctionType> FunctionType::parse(std::istream& in) {
    Expected<Byte> comp_type_exp = Byte::parse(in);
    if (!comp_type_exp)
        return Unexpected(PROPAGATE(comp_type_exp));
    uint8_t type = *comp_type_exp;

    if (type != 0x60)
        return Unexpected(ERROR("function type has the wrong composite type"));

    Expected<ExpectedType> param_types_exp = ExpectedType::parse(in);
    if (!param_types_exp)
        return Unexpected(PROPAGATE(param_types_exp));

    Expected<ExpectedType> result_types_exp = ExpectedType::parse(in);
    if (!result_types_exp)
        return Unexpected(PROPAGATE(result_types_exp));

    return FunctionType(*param_types_exp, *result_types_exp);
}

size_t FunctionType::getSignature() const {
    return std::hash<FunctionType>()(*this);
}

std::string FunctionType::toString() const {
    return fmt::format("{} -> {}", param_types_.toString(),
                       result_types_.toString());
}

FunctionType::FunctionType(const ExpectedType& param_types,
                           const ExpectedType& result_types)
    : param_types_(param_types), result_types_(result_types) {}

/**********/
/* Limits */
/**********/

Expected<Limits> Limits::parse(std::istream& in) {
    Expected<Byte> flags_exp = Byte::parse(in);
    if (!flags_exp)
        return Unexpected(PROPAGATE(flags_exp));
    uint8_t flags = *flags_exp;

    Expected<U32> min_exp = U32::parse(in);
    if (!min_exp)
        return Unexpected(PROPAGATE(min_exp));

    uint32_t min = *min_exp;
    uint32_t max = min;

    if (flags == 0x01 || flags == 0x03) {
        Expected<U32> max_exp = U32::parse(in);
        if (!max_exp)
            return Unexpected(PROPAGATE(max_exp));

        max = *max_exp;
    }

    return Limits(flags, min, max);
}

std::string Limits::toString() const {
    return fmt::format("initial={} max={}", min_, max_);
}

Limits::Limits(uint8_t flags, uint32_t min, uint32_t max)
    : flags_(flags), min_(min), max_(max) {}

/****************/
/* Memory Types */
/****************/

Expected<MemoryType> MemoryType::parse(std::istream& in) {
    Expected<Limits> limits_exp = Limits::parse(in);
    if (!limits_exp)
        return Unexpected(PROPAGATE(limits_exp));

    return MemoryType(*limits_exp);
}

std::string MemoryType::toString() const {
    return fmt::format("{} {}", Limits::toString(), isShared() ? "shared" : "");
}

MemoryType::MemoryType(const Limits& limits) : Limits(limits) {}

/***************/
/* Table Types */
/***************/

Expected<TableType> TableType::parse(std::istream& in) {
    Expected<ReferenceType> ref_type_exp = ReferenceType::parse(in);
    if (!ref_type_exp)
        return Unexpected(PROPAGATE(ref_type_exp));

    Expected<Limits> limits_exp = Limits::parse(in);
    if (!limits_exp)
        return Unexpected(PROPAGATE(limits_exp));

    return TableType(*ref_type_exp, *limits_exp);
}

std::string TableType::toString() const {
    return fmt::format("{} {}", ReferenceType::toString(), Limits::toString());
}

TableType::TableType(const ReferenceType& ref_type, const Limits& limits)
    : ReferenceType(ref_type), Limits(limits) {}

/****************/
/* Global Types */
/****************/

Expected<GlobalType> GlobalType::parse(std::istream& in) {
    Expected<ValueType> val_type_exp = ValueType::parse(in);
    if (!val_type_exp)
        return Unexpected(PROPAGATE(val_type_exp));

    Expected<Byte> mut_exp = Byte::parse(in);
    if (!mut_exp)
        return Unexpected(PROPAGATE(mut_exp));

    return GlobalType(*val_type_exp, *mut_exp);
}

std::string GlobalType::toString() const {
    return fmt::format("{} mutable={}", ValueType::toString(), mut_);
}

GlobalType::GlobalType(const ValueType& val_type, uint8_t mut)
    : ValueType(val_type), mut_(mut) {}
