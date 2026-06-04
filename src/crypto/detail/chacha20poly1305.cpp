// ---------------------------------------------------------------------------
// ChaCha20-Poly1305 IETF AEAD (RFC 8439) — vendored, public-domain primitives.
//
// ChaCha20: from-scratch RFC 8439 reference implementation.
// Poly1305:  derived from Andrew Moon's "poly1305-donna" (poly1305-donna-32.h),
//            https://github.com/floodyberry/poly1305-donna (commit e6ad6e0),
//            public domain. Rewritten with an explicit state struct (no opaque
//            buffer / aliasing cast) and fixed-width types; the constant-time
//            poly1305_verify is preserved verbatim.
//
// Compiled with relaxed warnings.
// ---------------------------------------------------------------------------

#include "chacha20poly1305.hpp"

#include <cstring>

namespace esphome::api::noise::detail {

namespace {

// ----------------------------- ChaCha20 (RFC 8439) -------------------------

inline std::uint32_t load32_le(const std::uint8_t* p) {
    return static_cast<std::uint32_t>(p[0]) | (static_cast<std::uint32_t>(p[1]) << 8) |
           (static_cast<std::uint32_t>(p[2]) << 16) | (static_cast<std::uint32_t>(p[3]) << 24);
}

inline void store32_le(std::uint8_t* p, const std::uint32_t v) {
    p[0] = static_cast<std::uint8_t>(v);
    p[1] = static_cast<std::uint8_t>(v >> 8);
    p[2] = static_cast<std::uint8_t>(v >> 16);
    p[3] = static_cast<std::uint8_t>(v >> 24);
}

inline std::uint32_t rotl32(const std::uint32_t x, const int n) {
    return (x << n) | (x >> (32 - n));
}

inline void quarter_round(std::uint32_t& a, std::uint32_t& b, std::uint32_t& c, std::uint32_t& d) {
    a += b;
    d ^= a;
    d = rotl32(d, 16);
    c += d;
    b ^= c;
    b = rotl32(b, 12);
    a += b;
    d ^= a;
    d = rotl32(d, 8);
    c += d;
    b ^= c;
    b = rotl32(b, 7);
}

// Produce one 64-byte ChaCha20 keystream block for the given counter.
void chacha20_block(const std::uint8_t key[32],
                    const std::uint32_t counter,
                    const std::uint8_t nonce[12],
                    std::uint8_t out[64]) {
    std::uint32_t state[16];
    state[0] = 0x61707865;
    state[1] = 0x3320646e;
    state[2] = 0x79622d32;
    state[3] = 0x6b206574;
    for (int i = 0; i < 8; ++i) {
        state[4 + i] = load32_le(key + 4 * i);
    }
    state[12] = counter;
    state[13] = load32_le(nonce + 0);
    state[14] = load32_le(nonce + 4);
    state[15] = load32_le(nonce + 8);

    std::uint32_t working[16];
    std::memcpy(working, state, sizeof(state));

    for (int i = 0; i < 10; ++i) {
        quarter_round(working[0], working[4], working[8], working[12]);
        quarter_round(working[1], working[5], working[9], working[13]);
        quarter_round(working[2], working[6], working[10], working[14]);
        quarter_round(working[3], working[7], working[11], working[15]);
        quarter_round(working[0], working[5], working[10], working[15]);
        quarter_round(working[1], working[6], working[11], working[12]);
        quarter_round(working[2], working[7], working[8], working[13]);
        quarter_round(working[3], working[4], working[9], working[14]);
    }

    for (int i = 0; i < 16; ++i) {
        store32_le(out + 4 * i, working[i] + state[i]);
    }
}

// XOR `len` bytes of keystream (starting at block `counter`) into out = in ^ ks.
void chacha20_xor(const std::uint8_t key[32],
                  std::uint32_t counter,
                  const std::uint8_t nonce[12],
                  const std::uint8_t* in,
                  std::uint8_t* out,
                  const std::size_t len) {
    std::uint8_t block[64];
    std::size_t off = 0;
    while (off < len) {
        chacha20_block(key, counter, nonce, block);
        ++counter;
        std::size_t n = len - off;
        if (n > 64) {
            n = 64;
        }
        for (std::size_t i = 0; i < n; ++i) {
            out[off + i] = static_cast<std::uint8_t>(in[off + i] ^ block[i]);
        }
        off += n;
    }
}

// ----------------------------- Poly1305 (donna-32) -------------------------

struct Poly1305 {
    std::uint32_t r[5];
    std::uint32_t h[5];
    std::uint32_t pad[4];
    std::size_t leftover;
    std::uint8_t buffer[16];
    std::uint8_t final;
};

inline std::uint32_t u8to32(const std::uint8_t* p) {
    return load32_le(p);
}

void poly1305_init(Poly1305& st, const std::uint8_t key[32]) {
    // r &= 0xffffffc0ffffffc0ffffffc0fffffff
    st.r[0] = (u8to32(&key[0])) & 0x3ffffff;
    st.r[1] = (u8to32(&key[3]) >> 2) & 0x3ffff03;
    st.r[2] = (u8to32(&key[6]) >> 4) & 0x3ffc0ff;
    st.r[3] = (u8to32(&key[9]) >> 6) & 0x3f03fff;
    st.r[4] = (u8to32(&key[12]) >> 8) & 0x00fffff;

    st.h[0] = st.h[1] = st.h[2] = st.h[3] = st.h[4] = 0;

    st.pad[0] = u8to32(&key[16]);
    st.pad[1] = u8to32(&key[20]);
    st.pad[2] = u8to32(&key[24]);
    st.pad[3] = u8to32(&key[28]);

    st.leftover = 0;
    st.final = 0;
}

void poly1305_blocks(Poly1305& st, const std::uint8_t* m, std::size_t bytes) {
    const std::uint32_t hibit = st.final ? 0 : (1UL << 24);
    std::uint32_t r0, r1, r2, r3, r4;
    std::uint32_t s1, s2, s3, s4;
    std::uint32_t h0, h1, h2, h3, h4;
    std::uint64_t d0, d1, d2, d3, d4;
    std::uint32_t c;

    r0 = st.r[0];
    r1 = st.r[1];
    r2 = st.r[2];
    r3 = st.r[3];
    r4 = st.r[4];
    s1 = r1 * 5;
    s2 = r2 * 5;
    s3 = r3 * 5;
    s4 = r4 * 5;
    h0 = st.h[0];
    h1 = st.h[1];
    h2 = st.h[2];
    h3 = st.h[3];
    h4 = st.h[4];

    while (bytes >= 16) {
        // h += m[i]
        h0 += (u8to32(m + 0)) & 0x3ffffff;
        h1 += (u8to32(m + 3) >> 2) & 0x3ffffff;
        h2 += (u8to32(m + 6) >> 4) & 0x3ffffff;
        h3 += (u8to32(m + 9) >> 6) & 0x3ffffff;
        h4 += (u8to32(m + 12) >> 8) | hibit;

        // h *= r
        d0 = static_cast<std::uint64_t>(h0) * r0 + static_cast<std::uint64_t>(h1) * s4 +
             static_cast<std::uint64_t>(h2) * s3 + static_cast<std::uint64_t>(h3) * s2 +
             static_cast<std::uint64_t>(h4) * s1;
        d1 = static_cast<std::uint64_t>(h0) * r1 + static_cast<std::uint64_t>(h1) * r0 +
             static_cast<std::uint64_t>(h2) * s4 + static_cast<std::uint64_t>(h3) * s3 +
             static_cast<std::uint64_t>(h4) * s2;
        d2 = static_cast<std::uint64_t>(h0) * r2 + static_cast<std::uint64_t>(h1) * r1 +
             static_cast<std::uint64_t>(h2) * r0 + static_cast<std::uint64_t>(h3) * s4 +
             static_cast<std::uint64_t>(h4) * s3;
        d3 = static_cast<std::uint64_t>(h0) * r3 + static_cast<std::uint64_t>(h1) * r2 +
             static_cast<std::uint64_t>(h2) * r1 + static_cast<std::uint64_t>(h3) * r0 +
             static_cast<std::uint64_t>(h4) * s4;
        d4 = static_cast<std::uint64_t>(h0) * r4 + static_cast<std::uint64_t>(h1) * r3 +
             static_cast<std::uint64_t>(h2) * r2 + static_cast<std::uint64_t>(h3) * r1 +
             static_cast<std::uint64_t>(h4) * r0;

        // (partial) h %= p
        c = static_cast<std::uint32_t>(d0 >> 26);
        h0 = static_cast<std::uint32_t>(d0) & 0x3ffffff;
        d1 += c;
        c = static_cast<std::uint32_t>(d1 >> 26);
        h1 = static_cast<std::uint32_t>(d1) & 0x3ffffff;
        d2 += c;
        c = static_cast<std::uint32_t>(d2 >> 26);
        h2 = static_cast<std::uint32_t>(d2) & 0x3ffffff;
        d3 += c;
        c = static_cast<std::uint32_t>(d3 >> 26);
        h3 = static_cast<std::uint32_t>(d3) & 0x3ffffff;
        d4 += c;
        c = static_cast<std::uint32_t>(d4 >> 26);
        h4 = static_cast<std::uint32_t>(d4) & 0x3ffffff;
        h0 += c * 5;
        c = (h0 >> 26);
        h0 = h0 & 0x3ffffff;
        h1 += c;

        m += 16;
        bytes -= 16;
    }

    st.h[0] = h0;
    st.h[1] = h1;
    st.h[2] = h2;
    st.h[3] = h3;
    st.h[4] = h4;
}

void poly1305_update(Poly1305& st, const std::uint8_t* m, std::size_t bytes) {
    // handle leftover
    if (st.leftover) {
        std::size_t want = 16 - st.leftover;
        if (want > bytes) {
            want = bytes;
        }
        for (std::size_t i = 0; i < want; ++i) {
            st.buffer[st.leftover + i] = m[i];
        }
        bytes -= want;
        m += want;
        st.leftover += want;
        if (st.leftover < 16) {
            return;
        }
        poly1305_blocks(st, st.buffer, 16);
        st.leftover = 0;
    }

    // process full blocks
    if (bytes >= 16) {
        std::size_t want = bytes & ~static_cast<std::size_t>(15);
        poly1305_blocks(st, m, want);
        m += want;
        bytes -= want;
    }

    // store leftover
    if (bytes) {
        for (std::size_t i = 0; i < bytes; ++i) {
            st.buffer[st.leftover + i] = m[i];
        }
        st.leftover += bytes;
    }
}

void poly1305_finish(Poly1305& st, std::uint8_t mac[16]) {
    std::uint32_t h0, h1, h2, h3, h4, c;
    std::uint32_t g0, g1, g2, g3, g4;
    std::uint64_t f;
    std::uint32_t mask;

    // process the remaining block
    if (st.leftover) {
        std::size_t i = st.leftover;
        st.buffer[i++] = 1;
        for (; i < 16; ++i) {
            st.buffer[i] = 0;
        }
        st.final = 1;
        poly1305_blocks(st, st.buffer, 16);
    }

    // fully carry h
    h0 = st.h[0];
    h1 = st.h[1];
    h2 = st.h[2];
    h3 = st.h[3];
    h4 = st.h[4];

    c = h1 >> 26;
    h1 = h1 & 0x3ffffff;
    h2 += c;
    c = h2 >> 26;
    h2 = h2 & 0x3ffffff;
    h3 += c;
    c = h3 >> 26;
    h3 = h3 & 0x3ffffff;
    h4 += c;
    c = h4 >> 26;
    h4 = h4 & 0x3ffffff;
    h0 += c * 5;
    c = h0 >> 26;
    h0 = h0 & 0x3ffffff;
    h1 += c;

    // compute h + -p
    g0 = h0 + 5;
    c = g0 >> 26;
    g0 &= 0x3ffffff;
    g1 = h1 + c;
    c = g1 >> 26;
    g1 &= 0x3ffffff;
    g2 = h2 + c;
    c = g2 >> 26;
    g2 &= 0x3ffffff;
    g3 = h3 + c;
    c = g3 >> 26;
    g3 &= 0x3ffffff;
    g4 = h4 + c - (1UL << 26);

    // select h if h < p, or h + -p if h >= p
    mask = (g4 >> ((sizeof(std::uint32_t) * 8) - 1)) - 1;
    g0 &= mask;
    g1 &= mask;
    g2 &= mask;
    g3 &= mask;
    g4 &= mask;
    mask = ~mask;
    h0 = (h0 & mask) | g0;
    h1 = (h1 & mask) | g1;
    h2 = (h2 & mask) | g2;
    h3 = (h3 & mask) | g3;
    h4 = (h4 & mask) | g4;

    // h = h % (2^128)
    h0 = ((h0) | (h1 << 26)) & 0xffffffff;
    h1 = ((h1 >> 6) | (h2 << 20)) & 0xffffffff;
    h2 = ((h2 >> 12) | (h3 << 14)) & 0xffffffff;
    h3 = ((h3 >> 18) | (h4 << 8)) & 0xffffffff;

    // mac = (h + pad) % (2^128)
    f = static_cast<std::uint64_t>(h0) + st.pad[0];
    h0 = static_cast<std::uint32_t>(f);
    f = static_cast<std::uint64_t>(h1) + st.pad[1] + (f >> 32);
    h1 = static_cast<std::uint32_t>(f);
    f = static_cast<std::uint64_t>(h2) + st.pad[2] + (f >> 32);
    h2 = static_cast<std::uint32_t>(f);
    f = static_cast<std::uint64_t>(h3) + st.pad[3] + (f >> 32);
    h3 = static_cast<std::uint32_t>(f);

    store32_le(mac + 0, h0);
    store32_le(mac + 4, h1);
    store32_le(mac + 8, h2);
    store32_le(mac + 12, h3);

    // zero out the state
    std::memset(&st, 0, sizeof(st));
}

// Constant-time 16-byte tag comparison (poly1305-donna). Returns 1 if equal.
int poly1305_verify(const std::uint8_t mac1[16], const std::uint8_t mac2[16]) {
    unsigned int dif = 0;
    for (std::size_t i = 0; i < 16; ++i) {
        dif |= static_cast<unsigned int>(mac1[i] ^ mac2[i]);
    }
    dif = (dif - 1) >> ((sizeof(unsigned int) * 8) - 1);
    return static_cast<int>(dif);
}

// --------------------------- IETF AEAD composition -------------------------

const std::uint8_t zero_pad[16] = {0};

// Append `len` little-endian as a 64-bit value into the MAC.
void poly1305_update_u64_le(Poly1305& st, const std::uint64_t v) {
    std::uint8_t b[8];
    for (int i = 0; i < 8; ++i) {
        b[i] = static_cast<std::uint8_t>(v >> (8 * i));
    }
    poly1305_update(st, b, 8);
}

// Compute the AEAD Poly1305 tag over ad || pad || ciphertext || pad ||
// len(ad)_le64 || len(ct)_le64 using the one-time key derived for this nonce.
void aead_mac(const std::uint8_t key[32],
              const std::uint8_t nonce[12],
              const std::uint8_t* ad,
              const std::size_t ad_len,
              const std::uint8_t* ciphertext,
              const std::size_t ct_len,
              std::uint8_t tag[16]) {
    // Poly1305 one-time key = first 32 bytes of ChaCha20 keystream, counter 0.
    std::uint8_t poly_key_block[64];
    chacha20_block(key, 0, nonce, poly_key_block);

    Poly1305 st;
    poly1305_init(st, poly_key_block);

    if (ad_len) {
        poly1305_update(st, ad, ad_len);
        if (ad_len % 16) {
            poly1305_update(st, zero_pad, 16 - (ad_len % 16));
        }
    }
    if (ct_len) {
        poly1305_update(st, ciphertext, ct_len);
        if (ct_len % 16) {
            poly1305_update(st, zero_pad, 16 - (ct_len % 16));
        }
    }
    poly1305_update_u64_le(st, ad_len);
    poly1305_update_u64_le(st, ct_len);
    poly1305_finish(st, tag);
}

}  // namespace

void chacha20poly1305_encrypt(const std::uint8_t key[32],
                              const std::uint8_t nonce[12],
                              const std::uint8_t* ad,
                              const std::size_t ad_len,
                              const std::uint8_t* plaintext,
                              const std::size_t pt_len,
                              std::uint8_t* ciphertext,
                              std::uint8_t tag[16]) {
    // Message encryption uses the ChaCha20 keystream starting at counter 1.
    chacha20_xor(key, 1, nonce, plaintext, ciphertext, pt_len);
    aead_mac(key, nonce, ad, ad_len, ciphertext, pt_len, tag);
}

bool chacha20poly1305_decrypt(const std::uint8_t key[32],
                              const std::uint8_t nonce[12],
                              const std::uint8_t* ad,
                              const std::size_t ad_len,
                              const std::uint8_t* ciphertext,
                              const std::size_t ct_len,
                              const std::uint8_t tag[16],
                              std::uint8_t* plaintext) {
    std::uint8_t expected[16];
    aead_mac(key, nonce, ad, ad_len, ciphertext, ct_len, expected);
    if (poly1305_verify(expected, tag) != 1) {
        return false;
    }
    chacha20_xor(key, 1, nonce, ciphertext, plaintext, ct_len);
    return true;
}

}  // namespace esphome::api::noise::detail
