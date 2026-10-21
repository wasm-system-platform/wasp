#pragma once

#include "runtime/operations/operations.hpp"

namespace runtime {

class LocalGet : public GenericOperation<LocalGet> {
public:
    using InType = Values<0>;
    using OutType = Values<1>;

    static OutType impl(LocalGet& local_get, Instance& instance);

    LocalGet(const grammar::LocalGet& local_get)
        : GenericOperation<LocalGet>(local_get.getAddress()),
          local_idx_(local_get.getLocalIdx()) {}

private:
    uint32_t local_idx_;
};

class LocalSet : public GenericOperation<LocalSet> {
public:
    using InType = Values<1>;
    using OutType = Values<0>;

    static OutType impl(LocalSet& local_set, Instance& instance, Value in);

    LocalSet(const grammar::LocalSet& local_set)
        : GenericOperation<LocalSet>(local_set.getAddress()),
          local_idx_(local_set.getLocalIdx()) {}

private:
    uint32_t local_idx_;
};

class LocalTee : public GenericOperation<LocalTee> {
public:
    using InType = std::tuple<Value>;
    using OutType = Value;

    static OutType impl(LocalTee& local_tee, Instance& instance, Value in);

    LocalTee(const grammar::LocalTee& local_tee)
        : GenericOperation<LocalTee>(local_tee.getAddress()),
          local_idx_(local_tee.getLocalIdx()) {};

private:
    uint32_t local_idx_;
};

class GlobalGet : public TaggedOperation<GlobalGet> {
public:
    GlobalGet(const grammar::GlobalGet& global_get);

    Continuation action(Instance& instance) override;

private:
    uint32_t global_idx_;
};

class GlobalSet : public OperationBase {
public:
    GlobalSet(const grammar::GlobalSet& global_set);

    Continuation action(Instance& instance) override;

private:
    uint32_t global_idx_;
};

} // namespace runtime
