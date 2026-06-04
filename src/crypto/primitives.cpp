#include <esphome/api/crypto/primitives.hpp>
#include <esphome/api/exception.hpp>

#include <sodium.h>

#include <array>

namespace esphome::api::noise {

namespace {

using Nonce = std::array<unsigned char, crypto_aead_chacha20poly1305_ietf_NPUBBYTES>;

void build_nonce(const std::uint64_t counter, Nonce& out) {
    // Noise/ChaChaPoly: 4 zero bytes followed by the 64-bit counter, little-endian.
    out[0] = out[1] = out[2] = out[3] = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        out[4 + i] = static_cast<unsigned char>((counter >> (8 * i)) & 0xFF);
    }
}

}  // namespace

void ensure_init() {
    if (sodium_init() < 0) {
        throw EncryptionError("libsodium initialization failed");
    }
}

Hash sha256(const ByteView data) {
    Hash out{};
    crypto_hash_sha256(out.data(), data.data(), data.size());
    return out;
}

Hash hmac_sha256(const ByteView key, const ByteView data) {
    Hash out{};
    crypto_auth_hmacsha256_state state;
    crypto_auth_hmacsha256_init(&state, key.data(), key.size());
    crypto_auth_hmacsha256_update(&state, data.data(), data.size());
    crypto_auth_hmacsha256_final(&state, out.data());
    return out;
}

void hkdf(const Hash& chaining_key,
          const ByteView input_key_material,
          const int num_outputs,
          Hash& o1,
          Hash& o2,
          Hash& o3) {
    const Hash temp_key =
        hmac_sha256(ByteView(chaining_key.data(), chaining_key.size()), input_key_material);

    // output1 = HMAC(temp_key, 0x01)
    constexpr std::uint8_t one = 0x01;
    o1 = hmac_sha256(ByteView(temp_key.data(), temp_key.size()), ByteView(&one, 1));
    if (num_outputs == 1) {
        return;
    }

    // output2 = HMAC(temp_key, output1 || 0x02)
    ByteBuffer buf(o1.begin(), o1.end());
    buf.push_back(0x02);
    o2 = hmac_sha256(ByteView(temp_key.data(), temp_key.size()), buf);
    if (num_outputs == 2) {
        return;
    }

    // output3 = HMAC(temp_key, output2 || 0x03)
    buf.assign(o2.begin(), o2.end());
    buf.push_back(0x03);
    o3 = hmac_sha256(ByteView(temp_key.data(), temp_key.size()), buf);
}

PublicKey x25519_base(const PrivateKey& private_key) {
    PublicKey out{};
    crypto_scalarmult_base(out.data(), private_key.data());
    return out;
}

SymmetricKey x25519(const PrivateKey& private_key, const PublicKey& peer_public) {
    SymmetricKey out{};
    if (crypto_scalarmult(out.data(), private_key.data(), peer_public.data()) != 0) {
        throw EncryptionError("X25519 produced a degenerate shared secret");
    }
    return out;
}

void generate_keypair(PrivateKey& private_key, PublicKey& public_key) {
    randombytes_buf(private_key.data(), private_key.size());
    crypto_scalarmult_base(public_key.data(), private_key.data());
}

ByteBuffer aead_encrypt(const SymmetricKey& key,
                        const std::uint64_t nonce,
                        const ByteView ad,
                        const ByteView plaintext) {
    Nonce npub{};
    build_nonce(nonce, npub);

    ByteBuffer out(plaintext.size() + crypto_aead_chacha20poly1305_ietf_ABYTES);
    unsigned long long clen = 0;
    crypto_aead_chacha20poly1305_ietf_encrypt(out.data(),
                                              &clen,
                                              plaintext.data(),
                                              plaintext.size(),
                                              ad.data(),
                                              ad.size(),
                                              nullptr,
                                              npub.data(),
                                              key.data());
    out.resize(static_cast<std::size_t>(clen));
    return out;
}

bool aead_decrypt(const SymmetricKey& key,
                  const std::uint64_t nonce,
                  const ByteView ad,
                  const ByteView ciphertext,
                  ByteBuffer& out_plaintext) {
    if (ciphertext.size() < crypto_aead_chacha20poly1305_ietf_ABYTES) {
        return false;
    }
    Nonce npub{};
    build_nonce(nonce, npub);

    out_plaintext.resize(ciphertext.size() - crypto_aead_chacha20poly1305_ietf_ABYTES);
    unsigned long long mlen = 0;
    const int rc = crypto_aead_chacha20poly1305_ietf_decrypt(out_plaintext.data(),
                                                             &mlen,
                                                             nullptr,
                                                             ciphertext.data(),
                                                             ciphertext.size(),
                                                             ad.data(),
                                                             ad.size(),
                                                             npub.data(),
                                                             key.data());
    if (rc != 0) {
        out_plaintext.clear();
        return false;
    }
    out_plaintext.resize(static_cast<std::size_t>(mlen));
    return true;
}

bool base64_decode(const std::string& text, ByteBuffer& out) {
    out.assign(text.size(), 0);  // decoded size <= input size
    std::size_t out_len = 0;
    const int rc = sodium_base642bin(out.data(),
                                     out.size(),
                                     text.data(),
                                     text.size(),
                                     nullptr,
                                     &out_len,
                                     nullptr,
                                     sodium_base64_VARIANT_ORIGINAL);
    if (rc != 0) {
        out.clear();
        return false;
    }
    out.resize(out_len);
    return true;
}

}  // namespace esphome::api::noise
