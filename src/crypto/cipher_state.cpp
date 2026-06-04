#include <esphome/api/crypto/cipher_state.hpp>
#include <esphome/api/exception.hpp>

namespace esphome::api::noise {

void CipherState::initialize_key(const SymmetricKey& key) {
    key_ = key;
    nonce_ = 0;
    has_key_ = true;
}

ByteBuffer CipherState::encrypt_with_ad(const ByteView ad, const ByteView plaintext) {
    if (!has_key_) {
        return ByteBuffer(plaintext.begin(), plaintext.end());
    }
    ByteBuffer out = aead_encrypt(key_, nonce_, ad, plaintext);
    ++nonce_;
    return out;
}

ByteBuffer CipherState::decrypt_with_ad(const ByteView ad, const ByteView ciphertext) {
    if (!has_key_) {
        return ByteBuffer(ciphertext.begin(), ciphertext.end());
    }
    ByteBuffer out;
    if (!aead_decrypt(key_, nonce_, ad, ciphertext, out)) {
        throw EncryptionError("AEAD authentication failed (bad key or corrupt frame)");
    }
    ++nonce_;
    return out;
}

}  // namespace esphome::api::noise
