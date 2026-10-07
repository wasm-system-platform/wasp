#include <atomic>

#include "runtime/instance.hpp"
#include "runtime/kernel.hpp"
#include "runtime/process.hpp"

#include <tomcrypt.h>

namespace runtime {

namespace {

/* TODO: provide meaningful data */
static std::array<uint8_t, 32> secret;

struct Checkpoint {
    std::array<uint8_t, 12> nonce;
    std::array<uint8_t, 16> tag;
    uint32_t confidential_size;
    uint8_t confidential[];
};

class Encoder {
public:
    template <typename T>
        requires(std::is_trivially_copyable_v<T>)
    void push(T t) {
        size_t offset = data_.size();
        data_.resize(data_.size() + sizeof(T));
        std::memcpy(&data_.data()[offset], &t, sizeof(T));
    };

    std::vector<uint8_t> takeData() && { return std::move(data_); }

private:
    std::vector<uint8_t> data_;
};

std::vector<uint8_t> encode(Instance& instance) {
    Context& ctxt = instance.getActiveContext();

    Encoder encoder;

    /* Encode value stack. */
    Stack& stack = ctxt.getStack();

    size_t stack_size = stack.size();

    encoder.push(stack_size);
    for (size_t i = 0; i < stack_size; i++) {
        encoder.push(stack.data()[i]);
    }

    /* Encode function locals. */
    Locals& locals = ctxt.getLocals();

    encoder.push(locals.getStackPointer());
    encoder.push(locals.getFramePointer());

    const auto& values = locals.getValues();
    const auto& frames = locals.getFrames();

    encoder.push(values.size());
    for (size_t i = 0; i < values.size(); i++) {
        encoder.push(values.data()[i]);
    }

    encoder.push(frames.size());
    for (size_t i = 0; i < frames.size(); i++) {
        encoder.push(frames.data()[i]);
    }

    /* Encode control flow epilogues. */
    Epilogues& epilogues = ctxt.getEpilogues();

    const Operation* ep = ctxt.getEpilogues().data();
    for (size_t i = ctxt.getEpilogues().size() - 1; i > 0; i--) {
        const Operation& epilogue = ep[i];
        if (epilogue == nullptr)
            break;
    }

    size_t epilogues_size = epilogues.size();
    auto& cache = instance.as<Process>().getEpilogueCache();

    encoder.push(epilogues_size);
    for (size_t i = 0; i < epilogues_size; i++) {
        const Operation& op = epilogues.data()[i];
        cache.insert(op);
        encoder.push(op.get());
    }

    return std::move(encoder).takeData();
}

class Decoder {
public:
    explicit Decoder(std::span<const uint8_t> bytes) : bytes(bytes) {}

    template <typename T>
        requires(std::is_trivially_copyable_v<T>)
    T read() {
        assert(offset + sizeof(T) <= bytes.size());

        T t;
        std::memcpy(&t, bytes.data() + offset, sizeof(T));
        offset += sizeof(T);
        return t;
    }

private:
    std::span<const uint8_t> bytes;
    size_t offset = 0;
};

void decode(Instance& instance, const std::vector<uint8_t>& encoded) {
    Context& ctxt = instance.getActiveContext();

    Decoder decoder(encoded);

    /* Decode value stack. */
    Stack& stack = ctxt.getStack();

    auto stack_size = decoder.read<size_t>();

    stack.clear();
    for (size_t i = 0; i < stack_size; i++) {
        auto value = decoder.read<Value>();
        stack.push(value);
    }

    /* Decode function locals. */
    Locals& locals = ctxt.getLocals();

    auto stack_pointer = decoder.read<size_t>();
    auto frame_pointer = decoder.read<size_t>();

    locals.setStackPointer(stack_pointer);
    locals.setFramePointer(frame_pointer);

    std::vector<Value> values;
    auto num_values = decoder.read<size_t>();

    for (size_t i = 0; i < num_values; i++)
        values.push_back(decoder.read<Value>());

    std::vector<size_t> frames;
    auto num_frames = decoder.read<size_t>();

    for (size_t i = 0; i < num_frames; i++)
        frames.push_back(decoder.read<size_t>());

    locals.setFrames(std::move(frames));
    locals.setValues(std::move(values));

    /* Decode control flow epilogues. */
    Epilogues& epilogues = ctxt.getEpilogues();

    auto num_epilogues = decoder.read<size_t>();

    epilogues.clear();
    for (size_t i = 0; i < num_epilogues; i++) {
        Continuation continuation = decoder.read<Continuation>();

        if (continuation)
            epilogues.push(continuation->shared_from_this());
        else
            epilogues.push(nullptr);
    }

    const Operation* ep = ctxt.getEpilogues().data();
    for (size_t i = ctxt.getEpilogues().size() - 1; i > 0; i--) {
        const Operation& epilogue = ep[i];
        if (epilogue == nullptr)
            break;
    }
}

Errno encrypt(const std::vector<uint8_t>& plaintext,
              const std::array<uint8_t, 32>& key,
              const std::array<uint8_t, 12>& nonce,
              std::array<uint8_t, 16>& hash_out,
              std::span<uint8_t> confidential_out) {
    assert(!plaintext.empty());
    assert(confidential_out.size() >= plaintext.size());

    hash_out.fill(0);

    chacha20poly1305_state ctx;
    int result = chacha20poly1305_init(&ctx, key.data(),
                                       static_cast<unsigned long>(key.size()));
    if (result != CRYPT_OK)
        return Errno::io;

    result = chacha20poly1305_setiv(&ctx, nonce.data(),
                                    static_cast<unsigned long>(nonce.size()));
    if (result != CRYPT_OK)
        return Errno::io;

    result = chacha20poly1305_encrypt(
        &ctx, plaintext.data(), static_cast<unsigned long>(plaintext.size()),
        confidential_out.data());
    if (result != CRYPT_OK)
        return Errno::io;

    unsigned long tag_len = static_cast<unsigned long>(hash_out.size());
    result = chacha20poly1305_done(&ctx, hash_out.data(), &tag_len);
    if (result != CRYPT_OK)
        return Errno::io;

    if (tag_len != hash_out.size())
        return Errno::io;

    return Errno::success;
}

Errno decrypt(std::span<const uint8_t> confidential,
              const std::array<uint8_t, 32>& key,
              const std::array<uint8_t, 12>& nonce,
              const std::array<uint8_t, 16>& hash,
              std::vector<uint8_t>& plaintext_out) {
    assert(!confidential.empty());

    if (confidential.size() > std::numeric_limits<unsigned long>::max())
        return Errno::overflow;

    std::vector<uint8_t> plaintext(confidential.size());

    chacha20poly1305_state ctx;
    int result = chacha20poly1305_init(&ctx, key.data(),
                                       static_cast<unsigned long>(key.size()));
    if (result != CRYPT_OK)
        return Errno::io;

    result = chacha20poly1305_setiv(&ctx, nonce.data(),
                                    static_cast<unsigned long>(nonce.size()));
    if (result != CRYPT_OK)
        return Errno::io;

    result = chacha20poly1305_decrypt(
        &ctx, confidential.data(),
        static_cast<unsigned long>(confidential.size()), plaintext.data());
    if (result != CRYPT_OK)
        return Errno::io;

    std::array<uint8_t, 16> calculated_tag{};
    unsigned long tag_len = static_cast<unsigned long>(calculated_tag.size());
    result = chacha20poly1305_done(&ctx, calculated_tag.data(), &tag_len);

    if (result != CRYPT_OK || tag_len != calculated_tag.size())
        return Errno::io;

    if (mem_neq(calculated_tag.data(), hash.data(), hash.size()) != 0)
        return Errno::access;

    plaintext_out = std::move(plaintext);
    return Errno::success;
}

} // namespace

namespace checkpoint {

std::atomic<uint64_t> nonce_counter(0);

static Errno saveUnbuffered(Instance& instance,
                            std::span<uint8_t> checkpoint_out) {
    if (checkpoint_out.size() < sizeof(Checkpoint))
        return Errno::overflow;

    Checkpoint* state = reinterpret_cast<Checkpoint*>(checkpoint_out.data());

    std::vector<uint8_t> encoded_state = encode(instance);
    state->confidential_size = static_cast<uint32_t>(encoded_state.size());

    if (checkpoint_out.size() < (sizeof(Checkpoint) + encoded_state.size())) {
        return Errno::overflow;
    }

    *reinterpret_cast<uint64_t*>(state->nonce.data()) =
        nonce_counter.fetch_add(1);

    const std::array<uint8_t, 32>& key = secret;
    std::span<uint8_t> encrypted_state(
        state->confidential, checkpoint_out.size() - sizeof(Checkpoint));

    return encrypt(encoded_state, key, state->nonce, state->tag,
                   encrypted_state);
}

static Errno saveBuffered(Instance& instance, uint32_t checkpoint_offset,
                          uint32_t checkpoint_len) {
    if (checkpoint_len < sizeof(Checkpoint))
        return Errno::overflow;

    std::vector<uint8_t> encoded_state = encode(instance);
    if (checkpoint_len < (sizeof(Checkpoint) + encoded_state.size()))
        return Errno::overflow;

    std::vector<uint8_t> checkpoint_buffer(checkpoint_len);

    Checkpoint* checkpoint =
        reinterpret_cast<Checkpoint*>(checkpoint_buffer.data());
    checkpoint->confidential_size = static_cast<uint32_t>(encoded_state.size());
    *reinterpret_cast<uint64_t*>(checkpoint->nonce.data()) =
        nonce_counter.fetch_add(1);

    const std::array<uint8_t, 32>& key = secret;
    std::span<uint8_t> encrypted_state(checkpoint->confidential,
                                       checkpoint_len - sizeof(Checkpoint));

    Errno result = encrypt(encoded_state, key, checkpoint->nonce,
                           checkpoint->tag, encrypted_state);
    if (result != Errno::success)
        return result;

    auto& mmu = instance.as<Process>().getKernel().getMMU();
    if (!mmu.store(checkpoint_offset, checkpoint_buffer))
        return Errno::access;

    return Errno::success;
}

Errno save(Instance& instance, uint32_t checkpoint_offset,
           uint32_t checkpoint_len) {
    if (checkpoint_offset < hw::mem::VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.contains(checkpoint_offset, checkpoint_len)) {
            return Errno::invalid;
        }

        uint8_t* checkpoint_ptr;
        memory.ptr(checkpoint_offset, &checkpoint_ptr);
        std::span<uint8_t> checkpoint(checkpoint_ptr, checkpoint_len);

        return saveUnbuffered(instance, checkpoint);
    }

    return saveBuffered(instance, checkpoint_offset, checkpoint_len);
}

static Errno restoreUnbuffered(Instance& instance,
                               std::span<const uint8_t> checkpoint) {
    if (checkpoint.size() < sizeof(Checkpoint))
        return Errno::invalid;

    const Checkpoint* state =
        reinterpret_cast<const Checkpoint*>(checkpoint.data());
    if (state->confidential_size == 0 ||
        checkpoint.size() < sizeof(Checkpoint) + state->confidential_size)
        return Errno::invalid;

    std::span<const uint8_t> confidential(state->confidential,
                                          state->confidential_size);
    std::vector<uint8_t> encoded_state;

    Errno result =
        decrypt(confidential, secret, state->nonce, state->tag, encoded_state);
    if (result != Errno::success)
        return result;

    decode(instance, encoded_state);
    return Errno::success;
}

static Errno restoreBuffered(Instance& instance, uint32_t checkpoint_offset,
                             uint32_t checkpoint_len) {
    if (checkpoint_len < sizeof(Checkpoint))
        return Errno::invalid;

    std::vector<uint8_t> checkpoint_buffer(checkpoint_len);

    auto& mmu = instance.as<Process>().getKernel().getMMU();
    if (!mmu.load(checkpoint_offset, checkpoint_buffer))
        return Errno::access;

    const Checkpoint* state =
        reinterpret_cast<const Checkpoint*>(checkpoint_buffer.data());
    if (state->confidential_size == 0 ||
        checkpoint_buffer.size() <
            sizeof(Checkpoint) + state->confidential_size)
        return Errno::invalid;

    std::span<const uint8_t> confidential(state->confidential,
                                          state->confidential_size);
    std::vector<uint8_t> encoded_state;

    Errno result =
        decrypt(confidential, secret, state->nonce, state->tag, encoded_state);
    if (result != Errno::success)
        return result;

    decode(instance, encoded_state);
    return Errno::success;
}

Errno restore(Instance& instance, uint32_t checkpoint_offset,
              uint32_t checkpoint_len) {
    if (checkpoint_offset < hw::mem::VIRT_MEMORY) {
        Memory& memory = instance.getGlobalState().getMemory();
        if (!memory.contains(checkpoint_offset, checkpoint_len)) {
            return Errno::invalid;
        }

        const uint8_t* checkpoint_ptr;
        memory.ptr(checkpoint_offset, &checkpoint_ptr);
        std::span<const uint8_t> checkpoint(checkpoint_ptr, checkpoint_len);

        return restoreUnbuffered(instance, checkpoint);
    }

    return restoreBuffered(instance, checkpoint_offset, checkpoint_len);
}

} // namespace checkpoint

} // namespace runtime
