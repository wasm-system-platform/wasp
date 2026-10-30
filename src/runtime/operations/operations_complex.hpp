#pragma once

#include "runtime/operations/operations.hpp"
#include "runtime/operations/operations_control.hpp"

namespace runtime {

class LocalAddConstCall : public TaggedOperation<LocalAddConstCall> {
public:
    static constexpr uint32_t OPCODE = 0x20'41'6A'10;

    LocalAddConstCall(const grammar::LocalGet& get,
                      const grammar::I32Const& immediate,
                      const grammar::Call& call)
        : TaggedOperation<LocalAddConstCall>(get.getAddress()),
          local_idx_(get.getIndex()), immediate_(immediate.getVal()),
          func_idx_(call.getFuncIdx()),
          epilogue_(call.getFuncIdx(), call.getAddress()) {
        next_ = &epilogue_;
    }

    Continuation action(Instance& instance) override;

private:
    uint32_t local_idx_;
    int32_t immediate_;
    uint32_t func_idx_;
    Call::Epilogue epilogue_;
};

class I32LocalAddConst : public GenericOperation<I32LocalAddConst> {
public:
    using InType = Values<0>;
    using OutType = Values<0>;

    static constexpr uint32_t OPCODE = 0x20'41'6A'21;

    explicit I32LocalAddConst(const grammar::LocalGet& get,
                              const grammar::I32Const& immediate)
        : GenericOperation<I32LocalAddConst>(get.getAddress()),
          idx_(get.getIndex()), immediate_(immediate.getVal()) {}

    static OutType impl(I32LocalAddConst& self, Instance& instance);

private:
    uint32_t idx_;
    int32_t immediate_;
};

class I32LocalAddConstSet : public GenericOperation<I32LocalAddConstSet> {
public:
    using InType = Values<0>;
    using OutType = Values<0>;

    static constexpr uint32_t OPCODE = 0x20'41'6A'21;

    explicit I32LocalAddConstSet(const grammar::LocalGet& get,
                                 const grammar::I32Const& immediate,
                                 const grammar::LocalSet& set)
        : GenericOperation<I32LocalAddConstSet>(get.getAddress()),
          source_idx_(get.getIndex()), target_idx_(set.getIndex()),
          immediate_(immediate.getVal()) {}

    static OutType impl(I32LocalAddConstSet& self, Instance& instance);

private:
    uint32_t source_idx_;
    uint32_t target_idx_;
    int32_t immediate_;
};

class I32LocalAddConstTee : public GenericOperation<I32LocalAddConstTee> {
public:
    using InType = Values<0>;
    using OutType = Values<1>;

    static constexpr uint32_t OPCODE = 0x20'41'6A'22;

    explicit I32LocalAddConstTee(const grammar::LocalGet& get,
                                 const grammar::I32Const& immediate)
        : GenericOperation<I32LocalAddConstTee>(get.getAddress()),
          idx_(get.getIndex()), immediate_(immediate.getVal()) {}

    static OutType impl(I32LocalAddConstTee& self, Instance& instance);

private:
    uint32_t idx_;
    int32_t immediate_;
};

class I32LocalAddConstTeeTo : public GenericOperation<I32LocalAddConstTeeTo> {
public:
    using InType = Values<0>;
    using OutType = Values<1>;

    static constexpr uint32_t OPCODE = 0x20'41'6A'22;

    explicit I32LocalAddConstTeeTo(const grammar::LocalGet& get,
                                   const grammar::I32Const& immediate,
                                   const grammar::LocalTee& tee)
        : GenericOperation<I32LocalAddConstTeeTo>(get.getAddress()),
          source_idx_(get.getIndex()), target_idx_(tee.getIndex()),
          immediate_(immediate.getVal()) {}

    static OutType impl(I32LocalAddConstTeeTo& self, Instance& instance);

private:
    uint32_t source_idx_;
    uint32_t target_idx_;
    int32_t immediate_;
};

class LocalSubConstCall : public TaggedOperation<LocalSubConstCall> {
public:
    static constexpr uint32_t OPCODE = 0x20'41'6B'10;

    LocalSubConstCall(const grammar::LocalGet& get,
                      const grammar::I32Const& immediate,
                      const grammar::Call& call)
        : TaggedOperation<LocalSubConstCall>(get.getAddress()),
          local_idx_(get.getIndex()), immediate_(immediate.getVal()),
          func_idx_(call.getFuncIdx()),
          epilogue_(call.getFuncIdx(), call.getAddress()) {
        next_ = &epilogue_;
    }

    Continuation action(Instance& instance) override;

private:
    uint32_t local_idx_;
    int32_t immediate_;
    uint32_t func_idx_;
    Call::Epilogue epilogue_;
};

class I32LocalSubConst : public GenericOperation<I32LocalSubConst> {
public:
    using InType = Values<0>;
    using OutType = Values<0>;

    static constexpr uint32_t OPCODE = 0x20'41'6B'21;

    explicit I32LocalSubConst(const grammar::LocalGet& get,
                              const grammar::I32Const& immediate)
        : GenericOperation<I32LocalSubConst>(get.getAddress()),
          idx_(get.getIndex()), immediate_(immediate.getVal()) {}

    static OutType impl(I32LocalSubConst& self, Instance& instance);

private:
    uint32_t idx_;
    int32_t immediate_;
};

class I32LocalSubConstSet : public GenericOperation<I32LocalSubConstSet> {
public:
    using InType = Values<0>;
    using OutType = Values<0>;

    static constexpr uint32_t OPCODE = 0x20'41'6B'21;

    explicit I32LocalSubConstSet(const grammar::LocalGet& get,
                                 const grammar::I32Const& immediate,
                                 const grammar::LocalSet& set)
        : GenericOperation<I32LocalSubConstSet>(get.getAddress()),
          source_idx_(get.getIndex()), target_idx_(set.getIndex()),
          immediate_(immediate.getVal()) {}

    static OutType impl(I32LocalSubConstSet& self, Instance& instance);

private:
    uint32_t source_idx_;
    uint32_t target_idx_;
    int32_t immediate_;
};

class I32LocalSubConstTee : public GenericOperation<I32LocalSubConstTee> {
public:
    using InType = Values<0>;
    using OutType = Values<1>;

    static constexpr uint32_t OPCODE = 0x20'41'6B'22;

    explicit I32LocalSubConstTee(const grammar::LocalGet& get,
                                 const grammar::I32Const& immediate)
        : GenericOperation<I32LocalSubConstTee>(get.getAddress()),
          idx_(get.getIndex()), immediate_(immediate.getVal()) {}

    static OutType impl(I32LocalSubConstTee& self, Instance& instance);

private:
    uint32_t idx_;
    int32_t immediate_;
};

class I32LocalSubConstTeeTo : public GenericOperation<I32LocalSubConstTeeTo> {
public:
    using InType = Values<0>;
    using OutType = Values<1>;

    static constexpr uint32_t OPCODE = 0x20'41'6B'22;

    explicit I32LocalSubConstTeeTo(const grammar::LocalGet& get,
                                   const grammar::I32Const& immediate,
                                   const grammar::LocalTee& tee)
        : GenericOperation<I32LocalSubConstTeeTo>(get.getAddress()),
          source_idx_(get.getIndex()), target_idx_(tee.getIndex()),
          immediate_(immediate.getVal()) {}

    static OutType impl(I32LocalSubConstTeeTo& self, Instance& instance);

private:
    uint32_t source_idx_;
    uint32_t target_idx_;
    int32_t immediate_;
};

} // namespace runtime
