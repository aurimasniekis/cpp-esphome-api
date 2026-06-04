// ---------------------------------------------------------------------------
// X25519 (Curve25519 ECDH) — vendored, public-domain reference.
//
// Source: TweetNaCl (tweetnacl.c), version 20140427, public domain. Functions
//         crypto_scalarmult / crypto_scalarmult_base and their field-arithmetic
//         helpers, transcribed with fixed-width <cstdint> types and wrapped in
//         the project namespace. Constant-time; algorithm unchanged.
//
// Compiled with relaxed warnings.
// ---------------------------------------------------------------------------

#include "x25519.hpp"

namespace esphome::api::noise::detail {

namespace {

using u8 = std::uint8_t;
using i64 = std::int64_t;
using gf = i64[16];

const u8 base9[32] = {9};
const gf c121665 = {0xDB41, 1};

void car25519(gf o) {
    i64 c;
    for (int i = 0; i < 16; ++i) {
        o[i] += (1LL << 16);
        c = o[i] >> 16;
        o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
        // `c` may be negative; use multiplication, since a left shift of a
        // negative value is undefined behaviour (the upstream TweetNaCl `c << 16`
        // is UB and trips UBSan). `c * 65536` is the value-equivalent, well-
        // defined form — `c` is bounded so the product cannot overflow i64.
        o[i] -= c * 65536;
    }
}

void sel25519(gf p, gf q, const int b) {
    i64 t;
    i64 c = ~(b - 1);
    for (int i = 0; i < 16; ++i) {
        t = c & (p[i] ^ q[i]);
        p[i] ^= t;
        q[i] ^= t;
    }
}

void pack25519(u8* o, const gf n) {
    int b;
    gf m, t;
    for (int i = 0; i < 16; ++i) {
        t[i] = n[i];
    }
    car25519(t);
    car25519(t);
    car25519(t);
    for (int j = 0; j < 2; ++j) {
        m[0] = t[0] - 0xffed;
        for (int i = 1; i < 15; ++i) {
            m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
            m[i - 1] &= 0xffff;
        }
        m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
        b = (m[15] >> 16) & 1;
        m[14] &= 0xffff;
        sel25519(t, m, 1 - b);
    }
    for (int i = 0; i < 16; ++i) {
        o[2 * i] = t[i] & 0xff;
        o[2 * i + 1] = t[i] >> 8;
    }
}

void unpack25519(gf o, const u8* n) {
    for (int i = 0; i < 16; ++i) {
        o[i] = n[2 * i] + (static_cast<i64>(n[2 * i + 1]) << 8);
    }
    o[15] &= 0x7fff;
}

void add(gf o, const gf a, const gf b) {
    for (int i = 0; i < 16; ++i) {
        o[i] = a[i] + b[i];
    }
}

void sub(gf o, const gf a, const gf b) {
    for (int i = 0; i < 16; ++i) {
        o[i] = a[i] - b[i];
    }
}

void mul(gf o, const gf a, const gf b) {
    i64 t[31];
    for (int i = 0; i < 31; ++i) {
        t[i] = 0;
    }
    for (int i = 0; i < 16; ++i) {
        for (int j = 0; j < 16; ++j) {
            t[i + j] += a[i] * b[j];
        }
    }
    for (int i = 0; i < 15; ++i) {
        t[i] += 38 * t[i + 16];
    }
    for (int i = 0; i < 16; ++i) {
        o[i] = t[i];
    }
    car25519(o);
    car25519(o);
}

void sqr(gf o, const gf a) {
    mul(o, a, a);
}

void inv25519(gf o, const gf i) {
    gf c;
    for (int a = 0; a < 16; ++a) {
        c[a] = i[a];
    }
    for (int a = 253; a >= 0; --a) {
        sqr(c, c);
        if (a != 2 && a != 4) {
            mul(c, c, i);
        }
    }
    for (int a = 0; a < 16; ++a) {
        o[a] = c[a];
    }
}

}  // namespace

int x25519_scalarmult(u8 q[32], const u8* n, const u8* p) {
    u8 z[32];
    i64 x[80];
    i64 r;
    gf a, b, c, d, e, f;
    for (int i = 0; i < 31; ++i) {
        z[i] = n[i];
    }
    z[31] = (n[31] & 127) | 64;
    z[0] &= 248;
    unpack25519(x, p);
    for (int i = 0; i < 16; ++i) {
        b[i] = x[i];
        d[i] = a[i] = c[i] = 0;
    }
    a[0] = d[0] = 1;
    for (int i = 254; i >= 0; --i) {
        r = (z[i >> 3] >> (i & 7)) & 1;
        sel25519(a, b, static_cast<int>(r));
        sel25519(c, d, static_cast<int>(r));
        add(e, a, c);
        sub(a, a, c);
        add(c, b, d);
        sub(b, b, d);
        sqr(d, e);
        sqr(f, a);
        mul(a, c, a);
        mul(c, b, e);
        add(e, a, c);
        sub(a, a, c);
        sqr(b, a);
        sub(c, d, f);
        mul(a, c, c121665);
        add(a, a, d);
        mul(c, c, a);
        mul(a, d, f);
        mul(d, b, x);
        sqr(b, e);
        sel25519(a, b, static_cast<int>(r));
        sel25519(c, d, static_cast<int>(r));
    }
    for (int i = 0; i < 16; ++i) {
        x[i + 16] = a[i];
        x[i + 32] = c[i];
        x[i + 48] = b[i];
        x[i + 64] = d[i];
    }
    inv25519(x + 32, x + 32);
    mul(x + 16, x + 16, x + 32);
    pack25519(q, x + 16);
    return 0;
}

int x25519_scalarmult_base(u8 q[32], const u8* n) {
    return x25519_scalarmult(q, n, base9);
}

}  // namespace esphome::api::noise::detail
