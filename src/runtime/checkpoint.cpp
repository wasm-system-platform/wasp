#include <atomic>

#include <openssl/evp.h>

#include "runtime/instance.hpp"
#include "runtime/process.hpp"
#include "runtime/kernel.hpp"

namespace runtime {

namespace {

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
    assert(confidential_out.size() >= plaintext.size());
    assert(plaintext.size() <= INT_MAX);

    hash_out.fill(0);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr)
        return Errno::no_memory;

    if (EVP_EncryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr,
                           nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    if (EVP_EncryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) !=
        1) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    int len = 0;

    if (EVP_EncryptUpdate(ctx, confidential_out.data(), &len, plaintext.data(),
                          static_cast<int>(plaintext.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    int ciphertext_len = len;

    if (EVP_EncryptFinal_ex(ctx, confidential_out.data() + ciphertext_len,
                            &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    ciphertext_len += len;
    if (ciphertext_len != static_cast<int>(plaintext.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_GET_TAG,
                            static_cast<int>(hash_out.size()),
                            hash_out.data()) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    EVP_CIPHER_CTX_free(ctx);
    return Errno::success;
}

Errno decrypt(std::span<const uint8_t> confidential,
              const std::array<uint8_t, 32>& key,
              const std::array<uint8_t, 12>& nonce,
              const std::array<uint8_t, 16>& hash,
              std::vector<uint8_t>& plaintext_out) {
    assert(!confidential.empty());

    if (confidential.size() > static_cast<size_t>(INT_MAX))
        return Errno::overflow;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (ctx == nullptr)
        return Errno::no_memory;

    std::vector<uint8_t> plaintext(confidential.size());

    if (EVP_DecryptInit_ex(ctx, EVP_chacha20_poly1305(), nullptr, nullptr,
                           nullptr) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    if (EVP_DecryptInit_ex(ctx, nullptr, nullptr, key.data(), nonce.data()) !=
        1) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    int len = 0;

    if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, confidential.data(),
                          static_cast<int>(confidential.size())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    int plaintext_len = len;

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_AEAD_SET_TAG,
                            static_cast<int>(hash.size()),
                            const_cast<uint8_t*>(hash.data())) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    if (EVP_DecryptFinal_ex(ctx, plaintext.data() + plaintext_len, &len) != 1) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::access;
    }

    plaintext_len += len;
    if (plaintext_len != static_cast<int>(confidential.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return Errno::io;
    }

    plaintext.resize(static_cast<size_t>(plaintext_len));
    plaintext_out = std::move(plaintext);

    EVP_CIPHER_CTX_free(ctx);
    return Errno::success;
}

} // namespace

namespace checkpoint {

Errno create(Instance& instance, std::span<uint8_t> checkpoint_out) {
    if (checkpoint_out.size() < sizeof(Checkpoint))
        return Errno::overflow;

    Checkpoint* state = reinterpret_cast<Checkpoint*>(checkpoint_out.data());

    std::vector<uint8_t> encoded_state = encode(instance);
    state->confidential_size = encoded_state.size();

    if (checkpoint_out.size() < (sizeof(Checkpoint) + encoded_state.size())) {
        return Errno::overflow;
    }

    static std::atomic<uint64_t> nonce_counter(0);
    *reinterpret_cast<uint64_t*>(state->nonce.data()) =
        nonce_counter.fetch_add(1);

    const std::array<uint8_t, 32>& key = instance.as<Process>().getKey();
    std::span<uint8_t> encrypted_state(
        state->confidential, checkpoint_out.size() - sizeof(Checkpoint));

    return encrypt(encoded_state, key, state->nonce, state->tag,
                   encrypted_state);
}

Errno restore(Instance& instance, std::span<const uint8_t> checkpoint) {
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

    Errno result = decrypt(confidential, instance.as<Process>().getKey(),
                           state->nonce, state->tag, encoded_state);
    if (result != Errno::success)
        return result;

    decode(instance, encoded_state);
    return Errno::success;
}

} // namespace checkpoint

} // namespace runtime
