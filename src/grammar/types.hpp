#pragma once

#include <cstdint>
#include <fstream>
#include <vector>

#include "util/error_handling.hpp"
#include "util/hash.hpp"

/******************/
/* Reference Type */
/******************/

struct ReferenceType {
public:
    static Expected<ReferenceType> parse(std::istream& in);

    std::string toString() const;

protected:
    enum class Type {
        funcref = 0x70,
        externref = 0x6F,
    } type_;

private:
    ReferenceType(Type type);
};

/**************/
/* Value Type */
/**************/

struct ValueType {
public:
    enum class Type {
        i32 = 0x7F,
        i64 = 0x7E,
        f32 = 0x7D,
        f64 = 0x7C,
        v128 = 0x7B,
        funcref = 0x70,
        externref = 0x6F,
    };

    ValueType(Type type) : type_(type) {}

    static Expected<ValueType> parse(std::istream& in);

    Type getType() const { return type_; }

    std::string toString() const;

private:
    Type type_;
};

struct ExpectedType {
public:
    static Expected<ExpectedType> parse(std::istream& in);

    ExpectedType(std::vector<ValueType>&& value_types)
        : value_types_(std::move(value_types)) {}

    const std::vector<ValueType>& getValueTypes() const { return value_types_; }

    std::string toString() const;

private:
    std::vector<ValueType> value_types_;
};

template <> struct std::hash<ExpectedType> {
    size_t operator()(const ExpectedType& expected_type) const noexcept {
        size_t seed = 0;
        for (const auto& value_type : expected_type.getValueTypes()) {
            hash_combine(seed, value_type.getType());
        }
        return seed;
    }
};

struct FunctionType {
public:
    static const FunctionType& Empty();             // () -> ()
    static const FunctionType& ConsumerI32();       // (i32) -> ()
    static const FunctionType& ConsumerI32I32();    // (i32, i32) -> ()
    static const FunctionType& ConsumerI32I32I32(); // (i32, i32, i32) -> ()
    static const FunctionType& Producer();          // () -> (i32)
    static const FunctionType& ProducerI32();       // (i32) -> (i32)
    static const FunctionType& ProducerI32I32();    // (i32, i32) -> (i32)
    static const FunctionType&
    ProducerI32_I32_I32_I32(); // (i32, i32, i32, i32) -> (i32)
    static const FunctionType&
    ProducerI32_I32_I32_I32_I32_I32(); // (i32, i32, i32, i32, i32, i32) ->
                                       // (i32)

    static Expected<FunctionType> parse(std::istream& in);

    const ExpectedType& getParamTypes() const { return param_types_; }
    const ExpectedType& getExpectedTypes() const { return result_types_; }
    const size_t getSignature() const;

    std::string toString() const;

private:
    ExpectedType param_types_;
    ExpectedType result_types_;

    FunctionType(const ExpectedType& param_types,
                 const ExpectedType& result_types);
};

template <> struct std::hash<FunctionType> {
    size_t operator()(const FunctionType& func_type) const noexcept {
        size_t seed = 0;
        hash_combine(seed, func_type.getExpectedTypes());
        hash_combine(seed, 0xCAFED00D);
        hash_combine(seed, func_type.getParamTypes());
        return seed;
    }
};

/**********/
/* Limits */
/**********/

struct Limits {
public:
    static Expected<Limits> parse(std::istream& in);

    std::string toString() const;

protected:
    uint8_t flags_;
    uint32_t min_;
    uint32_t max_;

    Limits(uint8_t flag, uint32_t min, uint32_t max);
};

/****************/
/* Memory Types */
/****************/

struct MemoryType : Limits {
public:
    static Expected<MemoryType> parse(std::istream& in);

    uint32_t getInitial() const { return min_; }

    uint32_t getMax() const { return max_; }

    bool isShared() const { return flags_ == 0x02 || flags_ == 0x03; }

    std::string toString() const;

private:
    MemoryType(const Limits& limits);
};

/***************/
/* Table Types */
/***************/

struct TableType : public ReferenceType, public Limits {
public:
    static Expected<TableType> parse(std::istream& in);

    std::string toString() const;

private:
    TableType(const ReferenceType& ref_type, const Limits& limits);
};

/****************/
/* Global Types */
/****************/

struct GlobalType : ValueType {
public:
    static Expected<GlobalType> parse(std::istream& in);

    std::string toString() const;

private:
    uint8_t mut_;

    GlobalType(const ValueType& val_type, uint8_t mut);
};
