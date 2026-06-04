#include <esphome/api/crypto/primitives.hpp>
#include <esphome/api/exception.hpp>

#include "detail/chacha20poly1305.hpp"
#include "detail/random.hpp"
#include "detail/sha256.hpp"
#include "detail/x25519.hpp"

#include <array>

namespace esphome::api::noise {

namespace {

// AEAD (IETF ChaCha20-Poly1305) parameters.
constexpr std::size_t aead_tag_len = detail::chacha20poly1305_tag_len;      // 16
constexpr std::size_t aead_nonce_len = detail::chacha20poly1305_nonce_len;  // 12

using Nonce = std::array<unsigned char, aead_nonce_len>;

void build_nonce(const std::uint64_t counter, Nonce& out) {
    // Noise/ChaChaPoly: 4 zero bytes followed by the 64-bit counter, little-endian.
    out[0] = out[1] = out[2] = out[3] = 0;
    for (std::size_t i = 0; i < 8; ++i) {
        out[4 + i] = static_cast<unsigned char>((counter >> (8 * i)) & 0xFF);
    }
}

int base64_value(const char c) {
    if (c >= 'A' && c <= 'Z') {
        return c - 'A';
    }
    if (c >= 'a' && c <= 'z') {
        return c - 'a' + 26;
    }
    if (c >= '0' && c <= '9') {
        return c - '0' + 52;
    }
    if (c == '+') {
        return 62;
    }
    if (c == '/') {
        return 63;
    }
    return -1;
}

}  // namespace

Hash sha256(const ByteView data) {
    Hash out{};
    detail::sha256(data.data(), data.size(), out.data());
    return out;
}

Hash hmac_sha256(const ByteView key, const ByteView data) {
    // HMAC-SHA-256 (RFC 2104) over the vendored streaming SHA-256.
    constexpr std::size_t block_len = 64;
    std::array<std::uint8_t, block_len> k_pad{};

    if (key.size() > block_len) {
        detail::sha256(key.data(), key.size(), k_pad.data());  // K' = H(key)
    } else {
        for (std::size_t i = 0; i < key.size(); ++i) {
            k_pad[i] = key[i];
        }
    }

    std::array<std::uint8_t, block_len> ipad{};
    std::array<std::uint8_t, block_len> opad{};
    for (std::size_t i = 0; i < block_len; ++i) {
        ipad[i] = static_cast<std::uint8_t>(k_pad[i] ^ 0x36);
        opad[i] = static_cast<std::uint8_t>(k_pad[i] ^ 0x5c);
    }

    // inner = H(ipad || data)
    detail::Sha256Ctx inner{};
    detail::sha256_init(inner);
    detail::sha256_update(inner, ipad.data(), ipad.size());
    detail::sha256_update(inner, data.data(), data.size());
    Hash inner_hash{};
    detail::sha256_final(inner, inner_hash.data());

    // out = H(opad || inner)
    detail::Sha256Ctx outer{};
    detail::sha256_init(outer);
    detail::sha256_update(outer, opad.data(), opad.size());
    detail::sha256_update(outer, inner_hash.data(), inner_hash.size());
    Hash out{};
    detail::sha256_final(outer, out.data());
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
    detail::x25519_scalarmult_base(out.data(), private_key.data());
    return out;
}

SymmetricKey x25519(const PrivateKey& private_key, const PublicKey& peer_public) {
    SymmetricKey out{};
    detail::x25519_scalarmult(out.data(), private_key.data(), peer_public.data());

    // Reject the all-zero shared secret (low-order peer point). Fold every byte
    // together in constant time so the check does not leak the secret.
    unsigned char acc = 0;
    for (const auto byte : out) {
        acc = static_cast<unsigned char>(acc | byte);
    }
    if (acc == 0) {
        throw EncryptionError("X25519 produced a degenerate shared secret");
    }
    return out;
}

void generate_keypair(PrivateKey& private_key, PublicKey& public_key) {
    detail::secure_random(private_key.data(), private_key.size());
    detail::x25519_scalarmult_base(public_key.data(), private_key.data());
}

ByteBuffer aead_encrypt(const SymmetricKey& key,
                        const std::uint64_t nonce,
                        const ByteView ad,
                        const ByteView plaintext) {
    Nonce npub{};
    build_nonce(nonce, npub);

    ByteBuffer out(plaintext.size() + aead_tag_len);
    detail::chacha20poly1305_encrypt(key.data(),
                                     npub.data(),
                                     ad.data(),
                                     ad.size(),
                                     plaintext.data(),
                                     plaintext.size(),
                                     out.data(),
                                     out.data() + plaintext.size());
    return out;
}

bool aead_decrypt(const SymmetricKey& key,
                  const std::uint64_t nonce,
                  const ByteView ad,
                  const ByteView ciphertext,
                  ByteBuffer& out_plaintext) {
    if (ciphertext.size() < aead_tag_len) {
        return false;
    }
    Nonce npub{};
    build_nonce(nonce, npub);

    const std::size_t ct_len = ciphertext.size() - aead_tag_len;
    out_plaintext.resize(ct_len);
    const bool ok = detail::chacha20poly1305_decrypt(key.data(),
                                                     npub.data(),
                                                     ad.data(),
                                                     ad.size(),
                                                     ciphertext.data(),
                                                     ct_len,
                                                     ciphertext.data() + ct_len,
                                                     out_plaintext.data());
    if (!ok) {
        out_plaintext.clear();
        return false;
    }
    return true;
}

bool base64_decode(const std::string& text, ByteBuffer& out) {
    out.clear();
    const std::size_t n = text.size();
    if (n % 4 != 0) {
        return false;
    }
    out.reserve(n / 4 * 3);

    for (std::size_t i = 0; i < n; i += 4) {
        const char c0 = text[i];
        const char c1 = text[i + 1];
        const char c2 = text[i + 2];
        const char c3 = text[i + 3];

        const int v0 = base64_value(c0);
        const int v1 = base64_value(c1);
        if (v0 < 0 || v1 < 0) {
            out.clear();
            return false;
        }
        out.push_back(static_cast<std::uint8_t>((v0 << 2) | (v1 >> 4)));

        if (c2 == '=') {
            // Only valid as the final quartet: "XX==" with no trailing data bits.
            if (i + 4 != n || c3 != '=' || (v1 & 0x0F) != 0) {
                out.clear();
                return false;
            }
            break;
        }
        const int v2 = base64_value(c2);
        if (v2 < 0) {
            out.clear();
            return false;
        }
        out.push_back(static_cast<std::uint8_t>((v1 << 4) | (v2 >> 2)));

        if (c3 == '=') {
            // Only valid as the final quartet: "XXX=" with no trailing data bits.
            if (i + 4 != n || (v2 & 0x03) != 0) {
                out.clear();
                return false;
            }
            break;
        }
        const int v3 = base64_value(c3);
        if (v3 < 0) {
            out.clear();
            return false;
        }
        out.push_back(static_cast<std::uint8_t>((v2 << 6) | v3));
    }
    return true;
}

}  // namespace esphome::api::noise
