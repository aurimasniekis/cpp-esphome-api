#include <esphome/api/crypto/symmetric_state.hpp>

#include <cstring>

namespace esphome::api::noise {

void SymmetricState::initialize(const std::string& protocol_name) {
    const ByteView name(reinterpret_cast<const std::uint8_t*>(protocol_name.data()),
                        protocol_name.size());
    if (protocol_name.size() <= hash_len) {
        h_.fill(0);
        std::memcpy(h_.data(), protocol_name.data(), protocol_name.size());
    } else {
        h_ = sha256(name);
    }
    ck_ = h_;
    cipher_ = CipherState{};  // no key yet
}

void SymmetricState::mix_key(const ByteView input) {
    Hash o1{};
    Hash o2{};
    Hash unused{};
    hkdf(ck_, input, 2, o1, o2, unused);
    ck_ = o1;
    cipher_.initialize_key(o2);
}

void SymmetricState::mix_hash(const ByteView data) {
    ByteBuffer buf(h_.begin(), h_.end());
    buf.insert(buf.end(), data.begin(), data.end());
    h_ = sha256(buf);
}

void SymmetricState::mix_key_and_hash(const ByteView input) {
    Hash o1{};
    Hash o2{};
    Hash o3{};
    hkdf(ck_, input, 3, o1, o2, o3);
    ck_ = o1;
    mix_hash(ByteView(o2.data(), o2.size()));
    cipher_.initialize_key(o3);
}

ByteBuffer SymmetricState::encrypt_and_hash(const ByteView plaintext) {
    ByteBuffer ciphertext = cipher_.encrypt_with_ad(ByteView(h_.data(), h_.size()), plaintext);
    mix_hash(ciphertext);
    return ciphertext;
}

ByteBuffer SymmetricState::decrypt_and_hash(const ByteView ciphertext) {
    ByteBuffer plaintext = cipher_.decrypt_with_ad(ByteView(h_.data(), h_.size()), ciphertext);
    mix_hash(ciphertext);
    return plaintext;
}

void SymmetricState::split(CipherState& first, CipherState& second) const {
    Hash o1{};
    Hash o2{};
    Hash unused{};
    hkdf(ck_, ByteView{}, 2, o1, o2, unused);
    first.initialize_key(o1);
    second.initialize_key(o2);
}

}  // namespace esphome::api::noise
